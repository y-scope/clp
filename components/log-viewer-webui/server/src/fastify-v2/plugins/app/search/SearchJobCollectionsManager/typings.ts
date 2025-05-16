/**
 * MongoDB document for search results.
 */
interface SearchResultsDocument {
    _id: string;
    orig_file_id: string;
    orig_file_path: string;
    log_event_ix: number;
    timestamp: number;
    message: string;
}

/**
 * Error thrown when a MongoDB collection has been dropped unexpectedly.
 */
class CollectionDroppedError extends Error {
    constructor (collectionName: string) {
        super(`Collection ${collectionName} has been dropped.`);
        this.name = "CollectionDroppedError";
    }
}

export {
    CollectionDroppedError, SearchResultsDocument,
};
