import {Meteor} from "meteor/meteor";

import {logger} from "/imports/utils/logger";

import {SearchResultsMetadataCollection} from "../collections";
import {
    SEARCH_MAX_NUM_RESULTS,
    SEARCH_SIGNAL,
} from "../constants";
import {ERROR_NAME_COLLECTION_DROPPED} from "../SearchJobCollectionsManager";
import {searchJobCollectionsManager} from "./collections";
import QueryJobsDbManager from "./QueryJobsDbManager";


/**
 * @type {QueryJobsDbManager|null}
 */
let queryJobsDbManager = null;

/**
 * Initializes the QueryJobsDbManager.
 *
 * @param {import("mysql2/promise").Pool} sqlDbConnPool
 * @param {object} tableNames
 * @param {string} tableNames.queryJobsTableName
 * @throws {Error} on error.
 */
const initQueryJobsDbManager = (sqlDbConnPool, {queryJobsTableName}) => {
    queryJobsDbManager = new QueryJobsDbManager(sqlDbConnPool, {queryJobsTableName});
};

/**
 * Modifies the search results metadata for a given job ID.
 *
 * @param {object} filter
 * @param {number} filter.jobId
 * @param {string} filter.lastSignal
 * @param {SearchResultsMetadata} fields The fields to be updated in the search results metadata.
 */
const updateSearchResultsMeta = ({
    jobId,
    lastSignal,
}, fields) => {
    const filter = {
        _id: jobId.toString(),
        lastSignal: lastSignal,
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
 * @param {object} props
 * @param {number} props.searchJobId of the job to monitor
 * @param {number} props.aggregationJobId of the job to monitor
 */
const updateSearchSignalWhenJobsFinish = async ({
    searchJobId,
    aggregationJobId,
}) => {
    let errorMsg;
    try {
        await queryJobsDbManager.awaitJobCompletion(searchJobId);
        await queryJobsDbManager.awaitJobCompletion(aggregationJobId);
    } catch (e) {
        errorMsg = e.message;
    }

    let numResultsInCollection = -1;
    try {
        numResultsInCollection = await searchJobCollectionsManager
            .getOrCreateCollection(searchJobId)
            .countDocuments();
    } catch (e) {
        if (ERROR_NAME_COLLECTION_DROPPED === e.error) {
            logger.warn(`Collection ${searchJobId} has been dropped.`);

            return;
        }
        throw e;
    }

    updateSearchResultsMeta({
        jobId: searchJobId,
        lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
    }, {
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
    await queryJobRawCollection.createIndexes([
        timestampAscendingIndex,
        timestampDescendingIndex,
    ]);
};

Meteor.methods({
    /**
     * @typedef {object} SubmitQueryResp
     * @property {number} searchJobId
     * @property {number} aggregationJobId
     */
    /**
     * Submits a search query and initiates the search process.
     *
     * @param {object} props
     * @param {string} props.queryString
     * @param {number} props.timestampBegin
     * @param {number} props.timestampEnd
     * @param {boolean} props.ignoreCase
     * @param {number} props.timeRangeBucketSizeMillis
     * @return {SubmitQueryResp}
     */
    async "search.submitQuery" ({
        queryString,
        timestampBegin,
        timestampEnd,
        ignoreCase,
        timeRangeBucketSizeMillis,
    }) {
        this.unblock();

        const args = {
            query_string: queryString,

            begin_timestamp: timestampBegin,
            end_timestamp: timestampEnd,
            ignore_case: ignoreCase,
            max_num_results: SEARCH_MAX_NUM_RESULTS,
        };

        logger.info("search.submitQuery args =", args);

        let searchJobId;
        let aggregationJobId;
        try {
            searchJobId = await queryJobsDbManager.submitSearchJob(args);
            aggregationJobId =
                await queryJobsDbManager.submitAggregationJob(args, timeRangeBucketSizeMillis);
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
            await updateSearchSignalWhenJobsFinish({
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
     * @param {object} props
     * @param {number} props.searchJobId of the search results to clear
     * @param {number} props.aggregationJobId of the search results to clear
     */
    async "search.clearResults" ({
        searchJobId,
        aggregationJobId,
    }) {
        this.unblock();

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
     * @param {object} props
     * @param {number} props.searchJobId
     * @param {number} props.aggregationJobId
     */
    async "search.cancelOperation" ({
        searchJobId,
        aggregationJobId,
    }) {
        this.unblock();

        logger.info(`search.cancelOperation searchJobId=${searchJobId}, ` +
            `aggregationJobId=${aggregationJobId}`);

        try {
            await queryJobsDbManager.submitQueryCancellation(searchJobId);
            await queryJobsDbManager.submitQueryCancellation(aggregationJobId);
            updateSearchResultsMeta({
                jobId: searchJobId,
                lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
            }, {
                lastSignal: SEARCH_SIGNAL.RESP_DONE,
                errorMsg: "Query cancelled before it could be completed.",
            });
        } catch (e) {
            const errorMsg = `Failed to submit cancel request for searchJobId=${searchJobId},` +
                `aggregationJobId=${aggregationJobId}.`;

            logger.error(errorMsg, e.toString());
            throw new Meteor.Error("query-cancel-error", errorMsg);
        }
    },
});

export {initQueryJobsDbManager};
