import {logger} from "/imports/utils/logger";
import {Meteor} from "meteor/meteor";
import {SearchResultsMetadataCollection} from "../collections";
import {SearchSignal} from "../constants";
import {searchJobCollectionsManager} from "./collections";
import SearchJobsDbManager from "./SearchJobsDbManager";


/**
 * @type {SearchJobsDbManager|null}
 */
let searchJobsDbManager = null;

/**
 * @param {mysql.Connection} sqlDbConnection
 * @param {object} tableNames
 * @param {string} tableNames.searchJobsTableName
 * @throws {Error} on error.
 */
const initSearchJobsDbManager = (sqlDbConnection, {searchJobsTableName}) => {
    searchJobsDbManager = new SearchJobsDbManager(sqlDbConnection, {searchJobsTableName});
};

/**
 * Updates the search event when the specified job finishes.
 *
 * @param {number} jobId of the job to monitor
 */
const updateSearchEventWhenJobFinishes = async (jobId) => {
    let errorMsg;
    try {
        await searchJobsDbManager.awaitJobCompletion(jobId);
    } catch (e) {
        errorMsg = e.message;
    }
    const filter = {
        _id: jobId.toString(),
    };
    const modifier = {
        $set: {
            lastSignal: SearchSignal.RESP_DONE,
            errorMsg: errorMsg,
            numTotalResults:
                await searchJobCollectionsManager.getOrCreateCollection(jobId).countDocuments(),
        },
    };

    logger.debug("modifier = ", modifier);
    SearchResultsMetadataCollection.update(filter, modifier);
};

/**
 * Creates MongoDB indexes for a specific job's collection.
 *
 * @param {number} jobId used to identify the Mongo Collection to add indexes
 */
const createMongoIndexes = async (jobId) => {
    const timestampAscendingIndex = {
        key: {
            timestamp: 1,
            _id: 1,
        },
        name: "timestamp-ascending",
    };
    const timestampDescendingIndex = {
        key: {
            timestamp: -1,
            _id: -1,
        },
        name: "timestamp-descending",
    };

    const queryJobCollection = searchJobCollectionsManager.getOrCreateCollection(jobId);
    const queryJobRawCollection = queryJobCollection.rawCollection();
    await queryJobRawCollection.createIndexes([timestampAscendingIndex, timestampDescendingIndex]);
};

Meteor.methods({
    /**
     * Submits a search query and initiates the search process.
     *
     * @param {string} queryString
     * @param {number} timestampBegin
     * @param {number} timestampEnd
     * @param {boolean} ignoreCase
     * @returns {Object} containing {jobId} of the submitted search job
     */
    async "search.submitQuery"({
        queryString,
        timestampBegin,
        timestampEnd,
        ignoreCase,
    }) {
        const args = {
            query_string: queryString,
            begin_timestamp: timestampBegin,
            end_timestamp: timestampEnd,
            ignore_case: ignoreCase,
        };
        logger.info("search.submitQuery args =", args);

        let jobId;
        try {
            jobId = await searchJobsDbManager.submitQuery(args);
        } catch (e) {
            const errorMsg = "Unable to submit search job to the SQL database.";
            logger.error(errorMsg, e.toString());
            throw new Meteor.Error("query-submit-error", errorMsg);
        }

        SearchResultsMetadataCollection.insert({
            _id: jobId.toString(),
            lastSignal: SearchSignal.RESP_QUERYING,
            errorMsg: null,
        });

        Meteor.defer(async () => {
            await updateSearchEventWhenJobFinishes(jobId);
        });

        await createMongoIndexes(jobId);

        return {jobId};
    },

    /**
     * Clears the results of a search operation identified by jobId.
     *
     * @param {number} jobId of the search results to clear
     */
    async "search.clearResults"({
        jobId,
    }) {
        logger.info("search.clearResults jobId =", jobId);

        try {
            const resultsCollection = searchJobCollectionsManager.getOrCreateCollection(jobId);
            await resultsCollection.dropCollectionAsync();

            searchJobCollectionsManager.removeCollection(jobId);
        } catch (e) {
            const errorMsg = `Failed to clear search results for jobId ${jobId}.`;
            logger.error(errorMsg, e.toString());
            throw new Meteor.Error("clear-results-error", errorMsg);
        }
    },

    /**
     * Cancels an ongoing search operation identified by jobId.
     *
     * @param {number} jobId of the search operation to cancel
     */
    async "search.cancelOperation"({
        jobId,
    }) {
        logger.info("search.cancelOperation jobId =", jobId);

        try {
            await searchJobsDbManager.submitQueryCancellation(jobId);
        } catch (e) {
            const errorMsg = `Failed to submit cancel request for job ${jobId}.`;
            logger.error(errorMsg, e.toString());
            throw new Meteor.Error("query-cancel-error", errorMsg);
        }
    },
});

export {initSearchJobsDbManager};
