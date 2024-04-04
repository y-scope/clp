import {logger} from "/imports/utils/logger";
import {Meteor} from "meteor/meteor";
import {SearchResultsMetadataCollection} from "../collections";
import {
    SEARCH_MAX_NUM_RESULTS,
    SEARCH_SIGNAL,
} from "../constants";
import {searchJobCollectionsManager} from "./collections";
import SearchJobsDbManager from "./SearchJobsDbManager";


/**
 * @type {SearchJobsDbManager|null}
 */
let searchJobsDbManager = null;

/**
 * @param {import("mysql2/promise").Pool} sqlDbConnPool
 * @param {object} tableNames
 * @param {string} tableNames.searchJobsTableName
 * @throws {Error} on error.
 */
const initSearchJobsDbManager = (sqlDbConnPool, {searchJobsTableName}) => {
    searchJobsDbManager = new SearchJobsDbManager(sqlDbConnPool, {searchJobsTableName});
};

/**
 * Modifies the search results metadata for a given job ID.
 *
 * @param {number} jobId
 * @param {object} fields - The fields to be updated in the search results metadata.
 */
const updateSearchResultsMeta = (jobId, fields) => {
    const filter = {
        _id: jobId.toString(),
    };

    const modifier = {
        $set: fields,
    };

    logger.debug("SearchResultsMetadataCollection modifier = ", modifier);
    SearchResultsMetadataCollection.update(filter, modifier);
};

/**
 * Updates the search signal when the specified job finishes.
 *
 * @param {number} jobId of the job to monitor
 */
const updateSearchSignalWhenJobFinishes = async (jobId) => {
    let errorMsg;
    try {
        await searchJobsDbManager.awaitJobCompletion(jobId);
    } catch (e) {
        errorMsg = e.message;
    }

    const numResultsInCollection = await searchJobCollectionsManager
        .getOrCreateCollection(jobId)
        .countDocuments();

    updateSearchResultsMeta(jobId, {
        lastSignal: SEARCH_SIGNAL.RESP_DONE,
        errorMsg: errorMsg,
        numTotalResults: Math.min(
            numResultsInCollection,
            SEARCH_MAX_NUM_RESULTS
        ),
    });
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
        this.unblock();

        const args = {
            query_string: queryString,
            begin_timestamp: timestampBegin,
            end_timestamp: timestampEnd,
            max_num_results: SEARCH_MAX_NUM_RESULTS,
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
            lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
            errorMsg: null,
        });

        Meteor.defer(async () => {
            await updateSearchSignalWhenJobFinishes(jobId);
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
        this.unblock();

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
        this.unblock();

        logger.info("search.cancelOperation jobId =", jobId);

        try {
            await searchJobsDbManager.submitQueryCancellation(jobId);

            updateSearchResultsMeta(jobId, {
                lastSignal: SEARCH_SIGNAL.RESP_DONE,
                errorMsg: "Query cancelled before it could be completed."
            });
        } catch (e) {
            const errorMsg = `Failed to submit cancel request for job ${jobId}.`;
            logger.error(errorMsg, e.toString());
            throw new Meteor.Error("query-cancel-error", errorMsg);
        }
    },
});

export {initSearchJobsDbManager};
