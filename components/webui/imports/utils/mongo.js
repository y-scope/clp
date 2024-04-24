/**
 * Enum of Mongo Collection sort orders.
 *
 * @enum {string}
 */
const MONGO_SORT_ORDER = Object.freeze({
    ASCENDING: "asc",
    DESCENDING: "desc",
});

/**
 * The sort order for MongoDB queries using the "_id" field.
 *
 * @type {string[]}
 */
const MONGO_SORT_BY_ID = Object.freeze([
    "_id",
    MONGO_SORT_ORDER.DESCENDING,
]);

export {
    MONGO_SORT_BY_ID,
    MONGO_SORT_ORDER,
};
