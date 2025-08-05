import type {InsertManyResult} from "mongodb";

import type {InsertPrestoRowsToMongoProps} from "./typings.js";


/**
 * Converts a Presto result row (array of values) into an object, using the provided column
 * definitions to assign property names.
 *
 * @param row Array of values representing a single Presto result row.
 * @param columns Array of column definitions, each containing a `name` property.
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
 * @param props.mongoDb
 * @return Promise that resolves when the insertion is complete
 */
const insertPrestoRowsToMongo = ({
    columns,
    data,
    mongoDb,
    searchJobId,
}: InsertPrestoRowsToMongoProps): Promise<InsertManyResult<Document>> => {
    const collection = mongoDb.collection(searchJobId);
    const resultDocs = data.map((row) => prestoRowToObject(row, columns));
    return collection.insertMany(resultDocs);
};

export {insertPrestoRowsToMongo};
