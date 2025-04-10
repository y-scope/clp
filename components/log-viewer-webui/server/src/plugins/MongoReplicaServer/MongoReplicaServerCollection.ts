import {
    ChangeStream,
    Collection,
    Db,
} from "mongodb";


/**
 * // eslint-disable-next-line no-warning-comments
 * TODO: Improve this? Think about security (other queries should not be able to kick others
 *  offline; maybe add a ref count then), performance, and collision chances.
 *
 * @param query
 * @param options
 * @return
 */
const getQueryHash = (query: object, options: object): string => JSON.stringify({query, options});

class MongoReplicaServerCollection {
    private count: number;

    private collection: Collection;

    private watchers: Map<string, ChangeStream>;

    constructor (mongoDb: Db, collectionName: string) {
        this.count = 0;
        this.collection = mongoDb.collection(collectionName);
        this.watchers = new Map();
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
        this.count--;
    }

    /**
     * Check if the collection is being referenced.
     *
     * @return
     */
    isReferenced (): boolean {
        return 0 < this.count;
    }

    find (query: object, options: object) {
        return this.collection.find(query, options);
    }

    getWatcher (query: object, options: object) {
        const queryHash = getQueryHash(query, options);
        let watcher = this.watchers.get(queryHash);
        if ("undefined" === typeof watcher) {
            watcher = this.collection.watch([{$match: query}], options);
            this.watchers.set(queryHash, watcher);
        }

        return {queryHash, watcher};
    }

    removeWatcher (queryHash: string) {
        const watcher = this.watchers.get(queryHash);

        if (watcher) {
            // Close the change stream and handle any potential errors
            watcher.close().catch((err: unknown) => {
                console.error(`Error closing watcher for queryHash ${queryHash}:`, err);
            });
            this.watchers.delete(queryHash);
        } else {
            console.warn(`No watcher found for queryHash ${queryHash}`);
        }
    }
}

export default MongoReplicaServerCollection;
