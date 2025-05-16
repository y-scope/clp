import {CollectionDroppedError} from "../../../plugins/app/search/SearchJobCollectionsManager/typings.js";
import {SEARCH_SIGNAL} from "../../../plugins/app/search/SearchResultsMetadataCollection/typings.js";
import {
    CreateMongoIndexesProps,
    SEARCH_MAX_NUM_RESULTS,
    UpdateSearchResultsMetaProps,
    UpdateSearchSignalWhenJobsFinishProps,
} from "./typings.js";


/**
 * Modifies the search results metadata for a given job ID.
 *
 * @param props
 * @param props.fields
 * @param props.jobId
 * @param props.lastSignal
 * @param props.logger
 * @param props.searchResultsMetadataCollection
 */
const updateSearchResultsMeta = async ({
    fields,
    jobId,
    lastSignal,
    logger,
    searchResultsMetadataCollection,
}: UpdateSearchResultsMetaProps) => {
    const filter = {
        _id: jobId.toString(),
        lastSignal: lastSignal,
    };

    const modifier = {
        $set: fields,
    };

    logger.debug("SearchResultsMetadataCollection modifier = ", modifier);
    await searchResultsMetadataCollection.updateOne(filter, modifier);
};

/**
 * Updates the search signal when the specified job finishes.
 *
 * @param props
 * @param props.aggregationJobId
 * @param props.logger
 * @param props.queryJobsDbManager
 * @param props.searchJobId
 * @param props.searchJobCollectionsManager
 * @param props.searchResultsMetadataCollection
 */
const updateSearchSignalWhenJobsFinish = async ({
    aggregationJobId,
    queryJobsDbManager,
    logger,
    searchJobId,
    searchJobCollectionsManager,
    searchResultsMetadataCollection,

}: UpdateSearchSignalWhenJobsFinishProps) => {
    let errorMsg = null;
    try {
        await queryJobsDbManager.awaitJobCompletion(searchJobId);
        await queryJobsDbManager.awaitJobCompletion(aggregationJobId);
    } catch (e: unknown) {
        if (e instanceof Error) {
            errorMsg = e.message;
        }
        errorMsg = "Error while waiting for job completion";
    }

    let numResultsInCollection = -1;
    try {
        const collection = await searchJobCollectionsManager
            .getOrCreateCollection(searchJobId);

        numResultsInCollection = await collection.countDocuments();

        // Need this here in new method. I believe publication used to do this?
        await searchJobCollectionsManager.getOrCreateCollection(aggregationJobId);
    } catch (e: unknown) {
        if (e instanceof CollectionDroppedError) {
            logger.warn(`Collection ${searchJobId} has been dropped.`);

            return;
        }
        throw e;
    }

    await updateSearchResultsMeta({
        fields: {
            lastSignal: SEARCH_SIGNAL.RESP_DONE,
            errorMsg: errorMsg,
            numTotalResults: Math.min(
                numResultsInCollection,
                SEARCH_MAX_NUM_RESULTS
            ),
        },
        jobId: searchJobId,
        lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
        logger: logger,
        searchResultsMetadataCollection: searchResultsMetadataCollection,
    });
};

/**
 * Creates MongoDB indexes for a specific job's collection.
 *
 * @param props
 * @param props.logger
 * @param props.searchJobId
 * @param props.searchJobCollectionsManager
 */
const createMongoIndexes = async ({
    logger,
    searchJobId,
    searchJobCollectionsManager,
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

    const queryJobCollection = await searchJobCollectionsManager.getOrCreateCollection(searchJobId);
    logger.info(`Creating indexes for collection ${searchJobId}`);
    await queryJobCollection.createIndexes([
        timestampAscendingIndex,
        timestampDescendingIndex,
    ]);
};

export {
    createMongoIndexes,
    updateSearchResultsMeta,
    updateSearchSignalWhenJobsFinish,
};
