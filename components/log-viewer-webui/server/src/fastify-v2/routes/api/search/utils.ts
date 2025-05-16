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
 * @param props.jobId
 * @param props.lastSignal
 * @param props.SearchResultsMetadataCollection
 * @param props.logger
 * @param props.fields
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

/**
 * Updates the search signal when the specified job finishes.
 *
 * @param props
 * @param props.searchJobId
 * @param props.aggregationJobId
 * @param props.queryJobsDbManager
 * @param props.searchJobCollectionsManager
 * @param props.SearchResultsMetadataCollection
 * @param props.logger
 */
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

    updateSearchResultsMeta({
        jobId: searchJobId,
        lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
        SearchResultsMetadataCollection,
        logger,
        fields: {
            lastSignal: SEARCH_SIGNAL.RESP_DONE,
            errorMsg: errorMsg ?? null,
            numTotalResults: Math.min(
                numResultsInCollection,
                SEARCH_MAX_NUM_RESULTS
            ),
        },
    });
};

/**
 * Creates MongoDB indexes for a specific job's collection.
 *
 * @param props
 * @param props.searchJobId
 * @param props.searchJobCollectionsManager
 * @param props.logger
 */
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
