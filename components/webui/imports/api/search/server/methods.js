import {Meteor} from "meteor/meteor";
import msgpack from "@msgpack/msgpack";

import {JOB_STATUS_WAITING_STATES, JobStatus, SearchSignal} from "../constants";
import {SQL_CONNECTION} from "../sql";
import {getCollection, MY_MONGO_DB, SearchResultsMetadataCollection} from "../collections";


const SEARCH_JOBS_TABLE_NAME = "distributed_search_jobs";

const sleep = (s) => new Promise(r => setTimeout(r, s * 1000));


const submitQuery = async (args) => {
    let jobId = null;

    try {
        const [queryInsertResults] = await SQL_CONNECTION.query(`INSERT INTO ${SEARCH_JOBS_TABLE_NAME} (search_config)
                                                                 VALUES (?) `, [Buffer.from(msgpack.encode(args))]);
        jobId = queryInsertResults["insertId"];
    } catch (e) {
        console.error("Unable to submit query job to SQL DB", e);
    }

    return jobId;
};


const waitTillJobFinishes = async (jobId) => {
    let errorMsg = null;

    try {
        while (true) {
            const [rows, _] = await SQL_CONNECTION.query(`SELECT status
                                                          from ${SEARCH_JOBS_TABLE_NAME}
                                                          where id = ${jobId}`);
            const status = rows[0]["status"];
            if (!JOB_STATUS_WAITING_STATES.includes(status)) {
                console.debug(`Job ${jobId} exited with status = ${status}`);

                if (JobStatus.SUCCESS !== status) {
                    if (JobStatus.NO_MATCHING_ARCHIVE === status) {
                        errorMsg = "No matching archive by query string and time range";
                    } else {
                        errorMsg = `Job exited in an unexpected status=${status}: ${Object.keys(JobStatus)[status]}`;
                    }
                }
                break;
            }

            await sleep(0.5);
        }
    } catch (e) {
        errorMsg = `Error querying job status, jobId=${jobId}: ${e}`;
        console.error(errorMsg);
    }

    return errorMsg;
};

const cancelQuery = async (jobId) => {
    const [rows, _] = await SQL_CONNECTION.query(`UPDATE ${SEARCH_JOBS_TABLE_NAME}
                                                  SET status = ${JobStatus.CANCELLING}
                                                  WHERE id = (?)`, [jobId]);
};


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

    SearchResultsMetadataCollection.update(filter, modifier);
};

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

        jobId = await submitQuery(args);
        if (null !== jobId) {
            SearchResultsMetadataCollection.insert({
                _id: jobId.toString(),
                lastSignal: SearchSignal.RSP_SEARCHING,
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

    async "search.clearResults"({
                                    jobId
                                })
    {
        console.debug(`Got request to clear results. jobid  = ${jobId}`);

        const resultsCollection = getCollection(MY_MONGO_DB, jobId.toString());
        await resultsCollection.dropCollectionAsync();

        delete MY_MONGO_DB[jobId.toString()];
    },

    async "search.cancelOperation"({
                                       jobId
                                   }) {
        await cancelQuery(jobId);
    },
});
