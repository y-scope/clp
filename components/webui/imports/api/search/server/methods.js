import {logger} from "/imports/utils/logger";
import {Meteor} from "meteor/meteor";
import {SearchResultsMetadataCollection} from "../collections";
import {searchJobCollectionsManager} from "./collections";
import {SearchSignal} from "../constants";
import {cancelQuery, submitQuery, waitTillJobFinishes} from "./sql";

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
            lastSignal: SearchSignal.RESP_DONE,
            errorMsg: errorMsg,
            numTotalResults:
                await searchJobCollectionsManager.getOrCreateCollection(jobId).countDocuments(),
        }
    };

    logger.debug("modifier = ", modifier)
    SearchResultsMetadataCollection.update(filter, modifier);
};

/**
 * Creates MongoDB indexes for a specific job's collection.
 *
 * @param {number} jobId used to identify the Mongo Collection to add indexes
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
                lastSignal: SearchSignal.RESP_QUERYING,
                errorMsg: null
            });

            Meteor.defer(async () => {
                await updateSearchEventWhenJobFinishes(jobId);
            });

            await createMongoIndexes(jobId);
        }

        return {jobId};
    },

    /**
     * Clears the results of a search operation identified by jobId.
     *
     * @param {number} jobId of the search results to clear
     */
    async "search.clearResults"({
                                    jobId
                                })
    {
        logger.info("search.clearResults jobId =", jobId)

        try {
            const resultsCollection = searchJobCollectionsManager.getOrCreateCollection(jobId);
            await resultsCollection.dropCollectionAsync();

            searchJobCollectionsManager.removeCollection(jobId);
        } catch (e) {
            logger.error(`Unable to clear search results for jobId=${jobId}`, e);
        }
    },

    /**
     * Cancels an ongoing search operation identified by jobId.
     *
     * @param {number} jobId of the search operation to cancel
     */
    async "search.cancelOperation"({
                                       jobId
                                   }) {
        logger.info("search.cancelOperation jobId =", jobId)

        try {
            await cancelQuery(jobId);
        } catch (e) {
            logger.error(`Unable to cancel search operation for jobId=${jobId}`, e);
        }
    },
});
