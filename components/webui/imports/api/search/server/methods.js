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
 * @param {number} aggregationJobId of the job to monitor
 */
const updateSearchEventWhenJobFinishes = async (jobId, aggregationJobId) => {
    let errorMsg;
    try {
        await searchJobsDbManager.awaitJobCompletion(jobId);
        await searchJobsDbManager.awaitJobCompletion(aggregationJobId);
    } catch (e) {
        errorMsg = e.message;
    }

    const filter = {
        _id: jobId.toString(),
    };
    const numResultsInCollection = await searchJobCollectionsManager
        .getOrCreateCollection(jobId)
        .countDocuments();
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
     * @param {number} timeRangeBucketSizeMs
     * @return {Object}
     * @property {number} jobId of the submitted query
     * @property {number} aggregationJobId of the submitted query
     */
    async "search.submitQuery" ({
        queryString,
        timestampBegin,
        timestampEnd,
        ignoreCase,
        timeRangeBucketSizeMs,
    }) {
        const args = {
            query_string: queryString,
            begin_timestamp: timestampBegin,
            end_timestamp: timestampEnd,
            max_num_results: SEARCH_MAX_NUM_RESULTS,
            ignore_case: ignoreCase,
        };
        logger.info("search.submitQuery args =", args);

        let jobId;
        let aggregationJobId;
        try {
            jobId = await searchJobsDbManager.submitQuery(args);
            aggregationJobId = await searchJobsDbManager.submitAggregationJob(args, timeRangeBucketSizeMs);
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
            await updateSearchEventWhenJobFinishes(jobId, aggregationJobId);
        });

        await createMongoIndexes(jobId);

        return {jobId, aggregationJobId};
    },

    /**
     * Clears the results of a search operation identified by jobId.
     *
     * @param {number} jobId of the search results to clear
     */
    async "search.clearResults" ({
        jobId,
        aggregationJobId,
    }) {
        logger.info(`search.clearResults jobId=${jobId}, aggregationJobId=${aggregationJobId}`);

        try {
            const resultsCollection = searchJobCollectionsManager.getOrCreateCollection(jobId);
            await resultsCollection.dropCollectionAsync();

            const resultsAggregationCollection =
                searchJobCollectionsManager.getOrCreateCollection(aggregationJobId);
            await resultsAggregationCollection.dropCollectionAsync();
        } catch (e) {
            const errorMsg = `Failed to clear search results for jobId=${jobId}, ` +
                `aggregationJobId=${aggregationJobId}`;
            logger.error(errorMsg, e.toString());
            throw new Meteor.Error("clear-results-error", errorMsg);
        }
    },

    /**
     * Cancels an ongoing search operation identified by jobId.
     *
     * @param {number} jobId
     * @param {number} aggregationJobId
     */
    async "search.cancelOperation" ({
        jobId,
        aggregationJobId,
    }) {
        logger.info(`search.cancelOperation jobId=${jobId}, aggregationJobId=${aggregationJobId}`);

        try {
            await searchJobsDbManager.submitQueryCancellation(jobId);
            await searchJobsDbManager.submitQueryCancellation(aggregationJobId);
        } catch (e) {
            const errorMsg = `Failed to submit cancel request for jobId=${jobId},` +
                `aggregationJobId=${aggregationJobId}.`;

            logger.error(errorMsg, e.toString());
            throw new Meteor.Error("query-cancel-error", errorMsg);
        }
    },
});

export {initSearchJobsDbManager};
