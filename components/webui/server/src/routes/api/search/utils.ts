import {SEARCH_SIGNAL} from "@webui/common";
import type {Db} from "mongodb";

import {
    CreateMongoIndexesProps,
    SEARCH_MAX_NUM_RESULTS,
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
 * Updates the search signal when the specified job finishes.
 *
 * @param props
 * @param props.aggregationJobId
 * @param props.logger
 * @param props.queryJobDbManager
 * @param props.searchJobId
 * @param props.searchResultsMetadataCollection
 * @param props.mongoDb
 */
const updateSearchSignalWhenJobsFinish = async ({
    aggregationJobId,
    logger,
    mongoDb,
    queryJobDbManager,
    searchJobId,
    searchResultsMetadataCollection,

}: UpdateSearchSignalWhenJobsFinishProps) => {
    let errorMsg: string | null = null;

    try {
        await queryJobDbManager.awaitJobCompletion(searchJobId);
        await queryJobDbManager.awaitJobCompletion(aggregationJobId);
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

    const filter = {
        _id: searchJobId.toString(),
        lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
    };
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

    await searchResultsMetadataCollection.updateOne(filter, modifier);
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
    updateSearchSignalWhenJobsFinish,
};
