import {CLP_QUERY_ENGINES} from "../../../../../common/index.js";
import {updateSearchResultsMeta} from "../search/utils.js";
import type {
    InsertPrestoRowsToMongoProps,
    ProcessPrestoStateChangeProps,
} from "./typings.js";


/**
 * Converts a Presto row array to an object mapping column names to values.
 *
 * @param row
 * @param columns
 * @return An object mapping each column name to its corresponding value from the row.
 */
const prestoRowToObject = (
    row: unknown[],
    columns: {name: string}[]
): Record<string, unknown> => {
    const obj: Record<string, unknown> = {};
    columns.forEach((col, idx) => {
        obj[col.name] = row[idx];
    });

    return obj;
};

/**
 * Inserts Presto rows into a MongoDB collection for a given search job.
 *
 * @param props
 * @param props.data
 * @param props.columns
 * @param props.searchJobId
 * @param props.isResolved
 * @param props.mongoDb
 * @param props.log
 */
const insertPrestoRowsToMongo = ({
    columns,
    data,
    isResolved,
    log,
    mongoDb,
    searchJobId,
}: InsertPrestoRowsToMongoProps): void => {
    if (false === isResolved) {
        log.error("Presto data received before searchJobId was resolved; skipping insert.");

        return;
    }

    if (0 < data.length && searchJobId) {
        const collection = mongoDb.collection(searchJobId);
        const resultDocs = data.map((row) => prestoRowToObject(row, columns));
        collection.insertMany(resultDocs).catch((err: unknown) => {
            log.error(err, "Failed to insert Presto results into MongoDB");
        });
    }
};

/**
 * Processes Presto state changes - creates metadata on first call and resolves the promise,
 * then updates metadata on subsequent calls.
 *
 * @param props
 * @param props.queryId
 * @param props.state
 * @param props.isResolved
 * @param props.logger
 * @param props.searchResultsMetadataCollection
 * @param props.resolve
 * @return Updated isResolved flag
 */
const processPrestoStateChange = ({
    queryId,
    state,
    isResolved,
    logger,
    searchResultsMetadataCollection,
    resolve,
}: ProcessPrestoStateChangeProps): boolean => {
    // Insert metadata on first state callback
    if (false === isResolved) {
        searchResultsMetadataCollection.insertOne({
            _id: queryId,
            lastSignal: state,
            errorMsg: null,
            queryEngine: CLP_QUERY_ENGINES.PRESTO,
        }).catch((err: unknown) => {
            logger.error(err, "Failed to insert Presto metadata");
        });
        resolve(queryId);

        return true;
    }

    // Update lastSignal in metadata using Presto state names
    updateSearchResultsMeta({
        fields: {lastSignal: state},
        jobId: queryId,
        logger: logger,
        searchResultsMetadataCollection: searchResultsMetadataCollection,
    }).catch((err: unknown) => {
        logger.error(err, "Failed to update Presto metadata");
    });

    return isResolved;
};

export {
    insertPrestoRowsToMongo,
    processPrestoStateChange,
};
