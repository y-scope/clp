import { FastifyBaseLogger } from "fastify";
import { Collection } from "mongodb";

import { SearchResultsMetadataDocument } from '../../../plugins/app/search/SearchResultsMetadataCollection/typings.js';
import { SEARCH_SIGNAL } from '../../../plugins/app/search/constants.js';
import {
    UpdateSearchResultsMetaProps,
    UpdateSearchSignalWhenJobsFinishProps,
    CreateMongoIndexesProps,
} from './typings.js';

/**
 * Modifies the search results metadata for a given job ID.
 *
 * @param {UpdateSearchResultsMetaProps} params
 */
const updateSearchResultsMeta = ({
    jobId,
    lastSignal,
    SearchResultsMetadataCollection,
    logger,
    fields,
}: UpdateSearchResultsMetaProps) => {
    const filter = {
        _id: jobId.toString(),
        lastSignal: lastSignal,
    };

    const modifier = {
        $set: fields,
    };

    logger.debug("SearchResultsMetadataCollection modifier = ", modifier);
    SearchResultsMetadataCollection.updateOne(filter, modifier);
};

const updateSearchSignalWhenJobsFinish = async ({
    searchJobId,
    aggregationJobId,
    queryJobsDbManager,
    searchJobCollectionsManager,
    SearchResultsMetadataCollection,
    logger,
}: UpdateSearchSignalWhenJobsFinishProps) => {
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
        SearchResultsMetadataCollection,
        logger,
        fields: {
            lastSignal: SEARCH_SIGNAL.RESP_DONE,
            errorMsg: errorMsg,
            numTotalResults: Math.min(
                numResultsInCollection,
                SEARCH_MAX_NUM_RESULTS
            ),
        },
    });
};

const createMongoIndexes = async ({
    searchJobId,
    searchJobCollectionsManager,
    logger,
}: CreateMongoIndexesProps) => {
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
    logger.info(`Creating indexes for collection ${searchJobId}`);
    await queryJobRawCollection.createIndexes([
        timestampAscendingIndex,
        timestampDescendingIndex,
    ]);
};

export { updateSearchResultsMeta, updateSearchSignalWhenJobsFinish, createMongoIndexes };