import type {
    Collection,
    Db,
} from "mongodb";

import {
    CLIENT_UPDATE_TIMEOUT_MS,
    MongoCustomSocket,
    QueryId,
    QueryParameters,
    Watcher,
} from "./typings.js";
import {convertQueryToChangeStreamFormat} from "./utils.js";


/**
 * Provides watchers for MongoDB queries to a specific collection.
 */
class MongoWatcherCollection {
    #collection: Collection;

    // Active watchers
    #queryIdtoWatcherMap: Map<QueryId, Watcher> = new Map();

    /**
     * @param collectionName
     * @param io
     * @param mongoDb
     */
    constructor (collectionName: string, mongoDb: Db) {
        this.#collection = mongoDb.collection(collectionName);
    }

    /**
     * Checks if the collection is currently being referenced by any clients.
     *
     * @return True if the collection is referenced, false otherwise.
     */
    isReferenced (): boolean {
        return 0 !== this.#queryIdtoWatcherMap.size;
    }

    /**
     * Checks if a watcher exists for the given queryId.
     *
     * @param queryId
     * @return True if a watcher exists, false otherwise.
     */
    hasWatcher (
        queryId: QueryId,
    ): boolean {
        if ("undefined" === typeof this.#queryIdtoWatcherMap.get(queryId)) {
            return false;
        }

        return true;
    }


    /**
     * Unsubscribes a connection from a watcher. If the watcher has no more subscribers, it closes
     * the change stream.
     *
     * @param queryId
     * @param connectionId
     * @return True if connection is last subcriber, false otherwise.
     */
    unsubcribe (queryId: number, connectionId: string): boolean {
        const watcher = this.#queryIdtoWatcherMap.get(queryId);

        if ("undefined" === typeof watcher) {
            console.warn(`No watcher found for queryId ${queryId}`);

            return false;
        }
        console.log(`Length ${watcher.subscribers}`);

        if (1 < watcher.subscribers.length) {
            // Remove the first instance of the connectionId from the subscribers list. Note
            // there can be multiple instances of the same connectionId in the list.
            const index = watcher.subscribers.indexOf(connectionId);
            if (-1 !== index) {
                watcher.subscribers.splice(index, 1);
            }

            return false;
        }

        watcher.changeStream.close().catch((err: unknown) => {
            console.error(`Error closing watcher for queryId ${queryId}:`, err);
        });
        this.#queryIdtoWatcherMap.delete(queryId);
        return true;
    }

    /**
     * Adds connection to an existing watcher and joins the the room for the
     * given queryId.
     *
     * @param watcher
     * @param queryParameters
     * @param queryId
     * @param socket
     */
    async subscribe (
        queryId: QueryId,
        socket: MongoCustomSocket
    ) {
        const watcher = this.#queryIdtoWatcherMap.get(queryId);

        if ("undefined" === typeof watcher) {
            throw new Error(`No watcher found for queryId ${queryId}`);
        }
        console.log(`I subscribed ${socket.id}`)

        watcher.subscribers.push(socket.id);
        await socket.join(queryId.toString());
    }

    /**
     * Creates a new watcher for the given query and queryId.
     *
     * @param queryParams
     * @param queryId
     * @param emitUpdate
     * @return The newly created watcher.
     */
    createWatcher (
        queryParams: QueryParameters,
        queryId: QueryId,
        emitUpdate: (queryId: QueryId, data: object[]) => void
    ): void {
        const watcherQuery = convertQueryToChangeStreamFormat(queryParams.query);
        const mongoWatcher = this.#collection.watch(
            [{$match: watcherQuery}],
            {fullDocument: "updateLookup"}
        );

        const watcher: Watcher = {changeStream: mongoWatcher, subscribers: []};
        this.#setupWatcherListener(watcher, queryParams, queryId, emitUpdate);
        this.#queryIdtoWatcherMap.set(queryId, watcher);
    }

    /**
     * Sets up listener for the watcher to handle change events and emit updates to clients.
     *
     * @param watcher
     * @param queryParameters
     * @param queryId
     * @param emitUpdate
     */
    #setupWatcherListener (
        watcher: Watcher,
        queryParameters: QueryParameters,
        queryId: QueryId,
        emitUpdate: (queryId: QueryId, data: object[]) => void
    ) {
        let lastEmitTime = 0;
        let emitTimeout: NodeJS.Timeout | null = null;

        const emitUpdateWithTimeout = async () => {
            const currentTime = Date.now();
            if (CLIENT_UPDATE_TIMEOUT_MS <= currentTime - lastEmitTime) {
                lastEmitTime = currentTime;
                const data = await this.find(queryParameters);
                emitUpdate(queryId, data);
            } else if (!emitTimeout) {
                const delay = CLIENT_UPDATE_TIMEOUT_MS - (currentTime - lastEmitTime);
                emitTimeout = setTimeout(async () => {
                    emitTimeout = null;
                    const data = await this.find(queryParameters);
                    emitUpdate(queryId, data);
                    lastEmitTime = Date.now();
                }, delay);
            }
        };

        watcher.changeStream.on("change", () => {
            emitUpdateWithTimeout().catch((error: unknown) => {
                console.error("Error in emitUpdatesWithTimeout:", error);
            });
        });
    }

    /**
     * Emits updated data to clients subscribed to the given queryId.
     *
     * @param queryParameters
     * @return A promise that resolves to an array of documents.
     */
    async find (
        queryParameters: QueryParameters
    ): Promise<object[]> {
        const {query, options} = queryParameters;
        try {
            const documents = await this.#collection.find(query, options).toArray();
            return documents;
        } catch (error) {
            console.error("Error fetching data for update:", error);
            return [];
        }
    }
}

export default MongoWatcherCollection;