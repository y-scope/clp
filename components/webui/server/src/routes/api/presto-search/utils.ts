import type {PrestoRowObject} from "@webui/common";
import type {
    Db,
    InsertManyResult,
} from "mongodb";


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
): PrestoRowObject => {
    const obj: Record<string, unknown> = {};
    columns.forEach((col, idx) => {
        obj[col.name] = row[idx];
    });

    // Object is wrapped in a `row` property to prevent conflicts with MongoDB's `_id` field.
    return {row: obj};
};

/**
 * Inserts Presto rows into a MongoDB collection for a given search job.
 *
 * @param data Array of Presto result rows
 * @param columns Array of column definitions
 * @param searchJobId
 * @param mongoDb
 * @return Promise that resolves when the insertion is complete
 */
const insertPrestoRowsToMongo = (
    data: unknown[][],
    columns: {name: string}[],
    searchJobId: string,
    mongoDb: Db
): Promise<InsertManyResult> => {
    const collection = mongoDb.collection(searchJobId);
    const resultDocs = data.map((row) => prestoRowToObject(row, columns));
    return collection.insertMany(resultDocs);
};

export {insertPrestoRowsToMongo};
