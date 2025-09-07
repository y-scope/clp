/**
 * Presto row wrapped in a `row` property to prevent conflicts with MongoDB's `_id` field.
 */
interface PrestoRowObject {
    row: Record<string, unknown>;
}

/**
 * Presto search result in MongoDB.
 */
interface PrestoSearchResult extends PrestoRowObject {
    _id: string;
}

export type {
    PrestoRowObject,
    PrestoSearchResult
};

