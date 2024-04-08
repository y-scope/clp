import {Meteor} from "meteor/meteor";

import {logger} from "/imports/utils/logger";

import {SearchResultsMetadataCollection} from "../collections";
import {
    SEARCH_MAX_NUM_RESULTS,
    SEARCH_SIGNAL,
} from "../constants";
import {ERROR_NAME_COLLECTION_DROPPED} from "../SearchJobCollectionsManager";
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
 * Updates the search event when the specified job finishes.
 *
 * @param {number} searchJobId of the job to monitor
 * @param {number} aggregationJobId of the job to monitor
 */
const updateSearchEventWhenJobsFinish = async ({
    searchJobId,
    aggregationJobId,
}) => {
    let errorMsg;
    try {
        await searchJobsDbManager.awaitJobCompletion(searchJobId);
        await searchJobsDbManager.awaitJobCompletion(aggregationJobId);
    } catch (e) {
        errorMsg = e.message;
    }

    const filter = {
        _id: searchJobId.toString(),
    };

    let numResultsInCollection = -1;
    try {
        numResultsInCollection = await searchJobCollectionsManager
            .getOrCreateCollection(searchJobId)
            .countDocuments();
    } catch (e) {
        if (e.error === ERROR_NAME_COLLECTION_DROPPED) {
            logger.warn(`Collection ${searchJobId} has been dropped.`);

            return;
        } else {
            throw e;
        }
    }

    const modifier = {
        $set: {
            lastSignal: SEARCH_SIGNAL.RESP_DONE,
            errorMsg: errorMsg,
            numTotalResults: Math.min(
                numResultsInCollection,
                SEARCH_MAX_NUM_RESULTS
            ),
        },
    };

    logger.debug("modifier = ", modifier);
    SearchResultsMetadataCollection.update(filter, modifier);
};

/**
 * Creates MongoDB indexes for a specific job's collection.
 *
 * @param {number} searchJobId used to identify the Mongo Collection to add indexes
 */
const createMongoIndexes = async (searchJobId) => {
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

    const queryJobCollection = searchJobCollectionsManager.getOrCreateCollection(searchJobId);
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
     * @param {number} timeRangeBucketSizeMillis
     * @return {Object}
     * @property {number} searchJobId of the submitted query
     * @property {number} aggregationJobId of the submitted query
     */
    async "search.submitQuery" ({
        queryString,
        timestampBegin,
        timestampEnd,
        ignoreCase,
        timeRangeBucketSizeMillis,
    }) {
        const args = {
            query_string: queryString,
            begin_timestamp: timestampBegin,
            end_timestamp: timestampEnd,
            max_num_results: SEARCH_MAX_NUM_RESULTS,
            ignore_case: ignoreCase,
        };
        logger.info("search.submitQuery args =", args);

        let searchJobId;
        let aggregationJobId;
        try {
            searchJobId = await searchJobsDbManager.submitSearchJob(args);
            aggregationJobId =
                await searchJobsDbManager.submitAggregationJob(args, timeRangeBucketSizeMillis);
        } catch (e) {
            const errorMsg = "Unable to submit search/aggregation job to the SQL database.";
            logger.error(errorMsg, e.toString());
            throw new Meteor.Error("query-submit-error", errorMsg);
        }

        SearchResultsMetadataCollection.insert({
            _id: searchJobId.toString(),
            lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
            errorMsg: null,
        });

        Meteor.defer(async () => {
            await updateSearchEventWhenJobsFinish({
                searchJobId,
                aggregationJobId,
            });
        });

        await createMongoIndexes(searchJobId);

        return {searchJobId, aggregationJobId};
    },

    /**
     * Clears the results of a search operation identified by jobId.
     *
     * @param {number} searchJobId of the search results to clear
     * @param {number} aggregationJobId of the search results to clear
     */
    async "search.clearResults" ({
        searchJobId,
        aggregationJobId,
    }) {
        logger.info(`search.clearResults searchJobId=${searchJobId}, ` +
            `aggregationJobId=${aggregationJobId}`);

        try {
            await searchJobCollectionsManager.dropCollection(searchJobId);
            await searchJobCollectionsManager.dropCollection(aggregationJobId);
        } catch (e) {
            const errorMsg = `Failed to clear search results for searchJobId=${searchJobId}, ` +
                `aggregationJobId=${aggregationJobId}`;

            logger.error(errorMsg, e.toString());
            throw new Meteor.Error("clear-results-error", errorMsg);
        }
    },

    /**
     * Cancels an ongoing search operation identified by jobId.
     *
     * @param {number} searchJobId
     * @param {number} aggregationJobId
     */
    async "search.cancelOperation" ({
        searchJobId,
        aggregationJobId,
    }) {
        logger.info(`search.cancelOperation searchJobId=${searchJobId}, ` +
            `aggregationJobId=${aggregationJobId}`);

        try {
            await searchJobsDbManager.submitQueryCancellation(searchJobId);
            await searchJobsDbManager.submitQueryCancellation(aggregationJobId);
        } catch (e) {
            const errorMsg = `Failed to submit cancel request for searchJobId=${searchJobId},` +
                `aggregationJobId=${aggregationJobId}.`;

            logger.error(errorMsg, e.toString());
            throw new Meteor.Error("query-cancel-error", errorMsg);
        }
    },
});

export {initSearchJobsDbManager};
