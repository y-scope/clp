import {Meteor} from "meteor/meteor";
import msgpack from "@msgpack/msgpack";
import {logger} from "/imports/utils/logger";

import {JOB_STATUS_WAITING_STATES, JobStatus, SearchSignal} from "../constants";
import {SQL_CONNECTION} from "../sql";
import {getCollection, MY_MONGO_DB, SearchResultsMetadataCollection} from "../collections";


const SEARCH_JOBS_TABLE_NAME = "distributed_search_jobs";

/**
 * Creates a promise that resolves after a specified number of seconds.
 *
 * @param {number} seconds to wait before resolving the promise
 * @returns {Promise<void>} that resolves after the specified delay
 */
const sleep = (seconds) => new Promise(r => setTimeout(r, seconds * 1000));


/**
 * Submits a query job to the SQL database and returns the job ID.
 *
 * @param {Object} args containing the search configuration.
 * @returns {number|null} job ID on successful submission, or null in case of failure
 */
const submitQuery = async (args) => {
    let jobId = null;

    try {
        const [queryInsertResults] = await SQL_CONNECTION.query(`INSERT INTO ${SEARCH_JOBS_TABLE_NAME} (search_config)
                                                                 VALUES (?) `, [Buffer.from(msgpack.encode(args))]);
        jobId = queryInsertResults["insertId"];
    } catch (e) {
        logger.error("Unable to submit query job to SQL DB", e);
    }

    return jobId;
};

/**
 * Waits for a job to finish and retrieves its status from the database.
 *
 * @param {number} jobId of the job to monitor
 *
 * @returns {?string} null if the job completes successfully; an error message if the job exits
 * in an unexpected status or encounters an error during monitoring
 */
const waitTillJobFinishes = async (jobId) => {
    let errorMsg = null;

    try {
        while (true) {
            const [rows, _] = await SQL_CONNECTION.query(`SELECT status
                                                          from ${SEARCH_JOBS_TABLE_NAME}
                                                          where id = ${jobId}`);
            const status = rows[0]["status"];
            if (!JOB_STATUS_WAITING_STATES.includes(status)) {
                logger.info(`Job ${jobId} exited with status = ${status}`);

                if (JobStatus.SUCCESS !== status) {
                    errorMsg = `Job exited in an unexpected status=${status}: ${Object.keys(JobStatus)[status]}`;
                }
                break;
            }

            await sleep(0.5);
        }
    } catch (e) {
        errorMsg = `Error querying job status, jobId=${jobId}: ${e}`;
        logger.error(errorMsg);
    }

    return errorMsg;
};

/**
 * Cancels a job by updating its status to 'CANCELLING' in the database.
 *
 * @param {string} jobId of the job to be cancelled
 */
const cancelQuery = async (jobId) => {
    const [rows, _] = await SQL_CONNECTION.query(`UPDATE ${SEARCH_JOBS_TABLE_NAME}
                                                  SET status = ${JobStatus.CANCELLING}
                                                  WHERE id = (?)`, [jobId]);
};

/**
 * Updates the search event when the specified job finishes.
 *
 * @param {number} jobId of the job to monitor
 */
const updateSearchEventWhenJobFinishes = async (jobId) => {
    const errorMsg = await waitTillJobFinishes(jobId);
    const filter = {
        _id: jobId.toString()
    };
    const modifier = {
        $set: {
            lastSignal: SearchSignal.RSP_DONE,
            errorMsg: errorMsg,
            numTotalResults: await getCollection(MY_MONGO_DB, jobId).countDocuments()
        }
    };

    logger.debug("modifier = ", modifier)
    SearchResultsMetadataCollection.update(filter, modifier);
};

/**
 * Creates MongoDB indexes for a specific job's collection.
 *
 * @param {?number} jobId used to identify the Mongo Collection to add indexes
 */
const createMongoIndexes = async (jobId) => {
    const timestampAscendingIndex = {
        key: {timestamp: 1, _id: 1},
        name: "timestamp-ascending"
    };

    const timestampDescendingIndex = {
        key: {timestamp: -1, _id: -1},
        name: "timestamp-descending"
    };

    if (null !== jobId) {
        const queryJobCollection = getCollection(MY_MONGO_DB, jobId.toString());
        const queryJobRawCollection = queryJobCollection.rawCollection();
        await queryJobRawCollection.createIndexes([timestampAscendingIndex, timestampDescendingIndex]);
    }
};

Meteor.methods({
    /**
     * Submits a search query and initiates the search process.
     *
     * @param {string} queryString
     * @param {number} timestampBegin
     * @param {number} timestampEnd
     *
     * @returns {Object} containing {jobId} of the submitted search job
     */
    async "search.submitQuery"({
                                   queryString,
                                   timestampBegin,
                                   timestampEnd,
                               }) {
        let jobId = null;

        const args = {
            query_string: queryString,
            begin_timestamp: timestampBegin,
            end_timestamp: timestampEnd,
        };
        logger.info("search.submitQuery args =", args)

        jobId = await submitQuery(args);
        if (null !== jobId) {
            SearchResultsMetadataCollection.insert({
                _id: jobId.toString(),
                lastSignal: SearchSignal.RSP_QUERYING,
                errorMsg: null
            });

            Meteor.defer(async () => {
                await updateSearchEventWhenJobFinishes(jobId);
            });

            // Create indexes for ascending / descending sort
            await createMongoIndexes(jobId);
        }

        return {jobId};
    },

    /**
     * Clears the results of a search operation identified by jobId.
     *
     * @param {string} jobId of the search results to clear
     */
    async "search.clearResults"({
                                    jobId
                                })
    {
        logger.info("search.clearResults jobId =", jobId)

        const resultsCollection = getCollection(MY_MONGO_DB, jobId.toString());
        await resultsCollection.dropCollectionAsync();

        delete MY_MONGO_DB[jobId.toString()];
    },

    /**
     * Cancels an ongoing search operation identified by jobId.
     *
     * @param {string} jobId of the search operation to cancel
     */
    async "search.cancelOperation"({
                                       jobId
                                   }) {
        logger.info("search.cancelOperation jobId =", jobId)

        await cancelQuery(jobId);
    },
});
