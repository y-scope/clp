import {
    ChangeStream,
    Collection,
    Db,
    Document,
    type Filter,
    type FindOptions,
} from "mongodb";
import {Server} from "socket.io";

import {convertFindToChangeStreamQuery} from "./utils.js";


const updateTimeout = 500;

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

    private io: Server;

    // Map of active change streams keyed by queryId
    private watchers: Map<number, Watcher> = new Map();

    /**
     * Creates an instance of MongoReplicaServerCollection.
     *
     * @param collectionName The name of the collection to manage.
     * @param io The Socket.IO server instance.
     * @param mongoDb The MongoDB database instance.
     */
    constructor (collectionName: string, io: Server, mongoDb: Db) {
        this.count = 0;
        this.collection = mongoDb.collection(collectionName);
        this.io = io;
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
     * @param queryId The unique identifier for the query.
     * @param connectionId The socket.id of the connection requesting the watcher.
     * @return An object containing the query hash and the change stream watcher.
     */
    getWatcher (
        query: Record<string, unknown>,
        options: Document,
        queryId: number,
        connectionId: string
    ) {
        let watcher = this.watchers.get(queryId);
        if ("undefined" === typeof watcher) {
            const watcherQuery = convertFindToChangeStreamQuery(query);
            const mongoWatcher = this.collection.watch([{
                $match: watcherQuery,
            }]);

            watcher = {changeStream: mongoWatcher, subscribers: [connectionId]};
            this.watchers.set(queryId, watcher);
            let lastEmitTime = 0;
            let pendingUpdate = false;

            const emitUpdate = async () => {
                const currentTime = Date.now();

                if (updateTimeout <= currentTime - lastEmitTime) {
                    lastEmitTime = currentTime;
                    this.io.to(`${queryId}`).emit("collection::find::update", {
                        queryId: queryId,
                        data: await this.collection.find(query, options).toArray(),
                    });

                    return;
                }

                if (!pendingUpdate) {
                    pendingUpdate = true;
                    // eslint-disable-next-line @typescript-eslint/no-misused-promises
                    setTimeout(async () => {
                        try {
                            const data = await this.collection.find(query, options).toArray();
                            this.io.to(`${queryId}`).emit("collection::find::update", {
                                queryId: queryId,
                                data: data,
                            });
                        } catch (error) {
                            console.error("Error fetching data for final update:", error);
                        } finally {
                            pendingUpdate = false;
                        }
                    }, updateTimeout);
                }
            };

            // eslint-disable-next-line @typescript-eslint/no-misused-promises
            watcher.changeStream.on("change", emitUpdate);
        } else {
            watcher.subscribers.push(connectionId);
        }

        return watcher;
    }

    /**
     * Closes and removes a change stream (watcher) for a specific query hash.
     * Handles potential errors when closing the watcher.
     *
     * @param queryId The id of the query for which to remove the watcher.
     * @param connectionId The socket.id of the connection requesting to remove the watcher.
     * @return True if the watcher was removed, false otherwise.
     */
    removeWatcher (queryId: number, connectionId: string): boolean {
        const watcher = this.watchers.get(queryId);

        let removed = true;
        if (watcher) {
            if (1 < watcher.subscribers.length) {
                // Remove the connectionId from the subscribers list
                watcher.subscribers = watcher.subscribers.filter((id) => id !== connectionId);

                removed = false;
            } else {
                // Close the change stream and handle any potential errors
                watcher.changeStream.close().catch((err: unknown) => {
                    console.error(`Error closing watcher for queryId ${queryId}:`, err);
                });
                this.watchers.delete(queryId);
            }
        } else {
            console.warn(`No watcher found for queryId ${queryId}`);
        }

        return removed;
    }
}

export default MongoServerCollection;
