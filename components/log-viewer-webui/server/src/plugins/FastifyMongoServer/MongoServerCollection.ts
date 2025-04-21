import {
    ChangeStream,
    Collection,
    Db,
    Document,
    type Filter,
    type FindOptions,
} from "mongodb";


/**
 * // eslint-disable-next-line no-warning-comments
 * TODO: Improve this? Think about security (other queries should not be able to kick others
 *  offline; maybe add a ref count then), performance, and collision chances.
 *
 * Generates a unique hash for a given query and options.
 * This hash is used to identify and manage change streams for specific queries.
 *
 * @param query The query object to be hashed.
 * @param options The options object to be hashed.
 * @return A string representing the unique hash for the query and options.
 */
const getQueryHash = (query: object, options: object): string => JSON.stringify({query, options});

interface Watcher {
    // The change stream for the query
    changeStream: ChangeStream;

    // List of connection IDs that are subscribed to this watcher
    subscribers: string[];
}

/**
 * Class representing a MongoDB collection with support for change streams.
 * This class manages subscriptions to real-time updates for specific queries.
 */
class MongoServerCollection {
    // Reference count for active subscriptions
    private count: number;

    // MongoDB collection instance
    private collection: Collection;

    // Map of active change streams keyed by query hash
    private watchers: Map<string, Watcher> = new Map();

    /**
     * Creates an instance of MongoReplicaServerCollection.
     *
     * @param mongoDb The MongoDB database instance.
     * @param collectionName The name of the collection to manage.
     */
    constructor (mongoDb: Db, collectionName: string) {
        this.count = 0;
        this.collection = mongoDb.collection(collectionName);
    }

    /**
     * Increment the reference count;
     */
    refAdd () {
        this.count++;
    }

    /**
     * Decrement the reference count;
     */
    refRemove () {
        if (0 < this.count) {
            this.count--;
        } else {
            console.warn("Attempted to remove reference when count is already 0");
        }
    }

    /**
     * Checks if the collection is currently being referenced by any clients.
     *
     * @return True if the collection is referenced, false otherwise.
     */
    isReferenced (): boolean {
        return 0 < this.count;
    }

    /**
     * Executes a find operation on the collection with the provided query and options.
     *
     * @param query The query object to filter documents.
     * @param options The options for the find operation.
     * @return A cursor for the documents matching the query.
     */
    find (query: Filter<Document>, options: FindOptions) {
        return this.collection.find(query, options);
    }

    /**
     * Retrieves or creates a change stream (watcher) for a specific query.
     * If a watcher for the query exists, it returns it; otherwise, it creates a new watcher.
     *
     * @param query The query object to watch for changes.
     * @param options The options for the change stream.
     * @param connectionId The socket.id of the connection requesting the watcher.
     * @return An object containing the query hash and the change stream watcher.
     */
    getWatcher (query: object, options: Document, connectionId: string) {
        const queryHash = getQueryHash(query, options);

        let watcher = this.watchers.get(queryHash);
        if ("undefined" === typeof watcher) {
            const mongoWatcher = this.collection.watch([{$match: query}], options);
            watcher = {changeStream: mongoWatcher, subscribers: [connectionId]};
            this.watchers.set(queryHash, watcher);
        } else {
            watcher.subscribers.push(connectionId);
        }

        return {queryHash, watcher};
    }

    /**
     * Closes and removes a change stream (watcher) for a specific query hash.
     * Handles potential errors when closing the watcher.
     *
     * @param queryHash The hash of the query for which to remove the watcher.
     * @param connectionId The socket.id of the connection requesting to remove the watcher.
     * @return True if the watcher was removed, false otherwise.
     */
    removeWatcher (queryHash: string, connectionId: string): boolean {
        const watcher = this.watchers.get(queryHash);

        let removed = true;
        if (watcher) {
            if (1 < watcher.subscribers.length) {
                // Remove the connectionId from the subscribers list
                watcher.subscribers = watcher.subscribers.filter((id) => id !== connectionId);

                removed = false;
            } else {
                // Close the change stream and handle any potential errors
                watcher.changeStream.close().catch((err: unknown) => {
                    console.error(`Error closing watcher for queryHash ${queryHash}:`, err);
                });
                this.watchers.delete(queryHash);
            }
        } else {
            console.warn(`No watcher found for queryHash ${queryHash}`);
        }

        return removed;
    }
}

export default MongoServerCollection;
