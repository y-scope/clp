import type {Db} from "mongodb";

import {SEARCH_SIGNAL} from "../../../../../../common/index.js";
import {
    CreateMongoIndexesProps,
    SEARCH_MAX_NUM_RESULTS,
    UpdateSearchResultsMetaProps,
    UpdateSearchSignalWhenJobsFinishProps,
} from "./typings.js";


/**
 * Checks if a collection exists in the MongoDB database.
 *
 * @param mongoDb
 * @param collectionName
 * @return Whether the collection exists.
 */
const hasCollection = async (mongoDb: Db, collectionName: string): Promise<boolean> => {
    const collections = await mongoDb.listCollections().toArray();
    return collections.some((collection: {name: string}) => collection.name === collectionName);
};

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
 * @param props.searchResultsMetadataCollection
 * @param props.mongoDb
 */
const updateSearchSignalWhenJobsFinish = async ({
    aggregationJobId,
    logger,
    mongoDb,
    queryJobsDbManager,
    searchJobId,
    searchResultsMetadataCollection,

}: UpdateSearchSignalWhenJobsFinishProps) => {
    let errorMsg: string | null = null;

    try {
        await queryJobsDbManager.awaitJobCompletion(searchJobId);
        await queryJobsDbManager.awaitJobCompletion(aggregationJobId);
    } catch (e: unknown) {
        errorMsg = e instanceof Error ?
            e.message :
            "Error while waiting for job completion";
    }

    logger.info(
        {searchJobId, aggregationJobId, errorMsg},
        "Search job and aggregation job completed."
    );

    let numResultsInCollection: number;
    const searchResultCollectionName = searchJobId.toString();

    if (await hasCollection(mongoDb, searchResultCollectionName)) {
        mongoDb.collection(searchResultCollectionName);
        numResultsInCollection = await mongoDb.collection(
            searchResultCollectionName
        ).countDocuments();
    } else {
        // Client may have sent delete request, removing the mongoDb collection, before the job
        // finished.
        logger.warn({errorMsg, searchJobId}, "Collection missing in database.");

        return;
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
 * @param props.mongoDb
 * @param props.searchJobId
 */
const createMongoIndexes = async ({
    searchJobId,
    logger,
    mongoDb,
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

    const queryJobCollection = mongoDb.collection(searchJobId.toString());
    logger.info(`Creating indexes for collection ${searchJobId}`);
    await queryJobCollection.createIndexes([
        timestampAscendingIndex,
        timestampDescendingIndex,
    ]);
};

export {
    createMongoIndexes,
    hasCollection,
    updateSearchResultsMeta,
    updateSearchSignalWhenJobsFinish,
};
