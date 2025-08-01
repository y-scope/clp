import type {InsertPrestoRowsToMongoProps} from "./typings.js";


/**
 * Adds column names as keys to a Presto row array, returning an object.
 *
 * @param row The array of values from Presto.
 * @param columns The array of column definitions, each with a `name` property.
 * @return An object mapping column names to row values.
 */
const addKeysToPrestoRow = (
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
        const resultDocs = data.map((row) => addKeysToPrestoRow(row, columns));
        collection.insertMany(resultDocs).catch((err: unknown) => {
            log.error({err}, "Failed to insert Presto results into MongoDB");
        });
    }
};

export {
    addKeysToPrestoRow, insertPrestoRowsToMongo,
};
