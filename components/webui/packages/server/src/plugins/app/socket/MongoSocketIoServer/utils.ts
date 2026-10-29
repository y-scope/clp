import {
    Document,
    Filter,
} from "mongodb";

import {QueryParameters} from "./typings.js";


/**
 * Modifies query so that it can be used to filter change stream update events. Update events in
 * MongoDB return the document in the "fullDocument" field, so function prepends "fullDocument" to
 * each key in the query.
 *
 * Reference: https://www.mongodb.com/docs/manual/reference/change-events/update/#mongodb-data-update
 *
 * @param query
 * @return Modified query.
 */
const convertQueryToChangeStreamFormat = (
    query: Filter<Document>
): Filter<Document> => {
    const changeStreamQuery: Filter<Document> = {};
    for (const key in query) {
        if (Object.hasOwn(query, key)) {
            changeStreamQuery[`fullDocument.${key}`] = query[key] as unknown;
        }
    }

    return changeStreamQuery;
};

/**
 * Generates a unique hash for a given query parameters.
 *
 * @param queryParams
 * @return Unique hash for query parameters.
 */
const getQueryHash = (
    queryParams: QueryParameters
): string => {
    return JSON.stringify(queryParams);
};

/**
 * Recovers query parameters from the hash.
 *
 * @param queryHash
 * @return
 */
const getQuery = (
    queryHash: string
): QueryParameters => {
    const parsedValue: unknown = JSON.parse(queryHash);
    return parsedValue as QueryParameters;
};

/**
 * Removes the first instance of item from an array, if it exists.
 *
 * @param array
 * @param item
 */
const removeItemFromArray = <T>(
    array: T[],
    item: T
): void => {
    const index = array.indexOf(item);
    if (-1 !== index) {
        array.splice(index, 1);
    }
};

export {
    convertQueryToChangeStreamFormat,
    getQuery,
    getQueryHash,
    removeItemFromArray,
};
