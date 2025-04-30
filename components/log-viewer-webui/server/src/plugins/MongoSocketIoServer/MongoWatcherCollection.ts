import {
    Collection,
    Db,
} from "mongodb";
import {Server} from "socket.io";

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
    private collection: Collection;

    private io: Server;

    // Active watchers
    private queryIdtoWatcherMap: Map<QueryId, Watcher> = new Map();

    /**
     * @param collectionName
     * @param io
     * @param mongoDb
     */
    constructor (collectionName: string, io: Server, mongoDb: Db) {
        this.collection = mongoDb.collection(collectionName);
        this.io = io;
    }

    /**
     * Checks if the collection is currently being referenced by any clients.
     *
     * @return True if the collection is referenced, false otherwise.
     */
    isReferenced (): boolean {
        return 0 !== this.queryIdtoWatcherMap.size;
    }

    /**
     * Gets the watcher for a specific query. If the watcher does not exist, it creates a new one.
     *
     * @param queryParameters
     * @param queryId
     * @param socket
     */
    async getWatcher (
        queryParameters: QueryParameters,
        queryId: QueryId,
        socket: MongoCustomSocket,
    ) {
        let watcher = this.queryIdtoWatcherMap.get(queryId);

        if (watcher) {
            await this.#subscribeToWatcher(watcher, queryParameters, queryId, socket);

            return;
        }

        watcher = this.#createNewWatcher(queryParameters, queryId, socket);
        this.queryIdtoWatcherMap.set(queryId, watcher);
    }

    /**
     * Unsubscribes a connection from a watcher. If the watcher has no more subscribers, it closes
     * the change stream.
     *
     * @param queryId
     * @param connectionId
     * @return True if connection is last subcriber, false otherwise.
     */
    unsubscribeFromWatcher (queryId: number, connectionId: string): boolean {
        const watcher = this.queryIdtoWatcherMap.get(queryId);

        if ("undefined" === typeof watcher) {
            console.warn(`No watcher found for queryId ${queryId}`);

            return false;
        }

        if (1 < watcher.subscribers.length) {
            // Remove the connection ID from the subscribers list
            watcher.subscribers = watcher.subscribers.filter((id) => id !== connectionId);

            return false;
        }

        watcher.changeStream.close().catch((err: unknown) => {
            console.error(`Error closing watcher for queryId ${queryId}:`, err);
        });
        this.queryIdtoWatcherMap.delete(queryId);

        return true;
    }

    /**
     * Adds connection to an existing watcher and sends the current data.
     *
     * @param watcher
     * @param queryParameters
     * @param queryId
     * @param socket
     */
    async #subscribeToWatcher (
        watcher: Watcher,
        queryParameters: QueryParameters,
        queryId: QueryId,
        socket: MongoCustomSocket
    ) {
        const {query, options} = queryParameters;
        watcher.subscribers.push(socket.id);
        const data = await this.collection.find(query, options).toArray();
        socket.emit("collection::find::update", {queryId, data});
    }

    /**
     * Creates a new watcher for the given query and queryId.
     *
     * @param queryParams
     * @param queryId
     * @param socket
     * @return The newly created watcher.
     */
    #createNewWatcher (
        queryParams: QueryParameters,
        queryId: QueryId,
        socket: MongoCustomSocket
    ): Watcher {
        const watcherQuery = convertQueryToChangeStreamFormat(queryParams.query);
        const mongoWatcher = this.collection.watch(
            [{$match: watcherQuery}],
            {fullDocument: "updateLookup"}
        );

        const watcher: Watcher = {changeStream: mongoWatcher, subscribers: [socket.id]};
        this.#setupWatcherListener(watcher, queryParams, queryId);

        return watcher;
    }

    /**
     * Sets up listener for the watcher to handle change events and emit updates to clients.
     *
     * @param watcher
     * @param queryParameters
     * @param queryId
     */
    #setupWatcherListener (
        watcher: Watcher,
        queryParameters: QueryParameters,
        queryId: QueryId
    ) {
        let lastEmitTime = 0;
        let emitTimeout: NodeJS.Timeout | null = null;

        const emitUpdate = async () => {
            const currentTime = Date.now();
            if (CLIENT_UPDATE_TIMEOUT_MS <= currentTime - lastEmitTime) {
                lastEmitTime = currentTime;
                await this.#emitDataUpdate(queryParameters, queryId);
            } else if (!emitTimeout) {
                const delay = CLIENT_UPDATE_TIMEOUT_MS - (currentTime - lastEmitTime);
                emitTimeout = setTimeout(() => {
                    emitTimeout = null;

                    this.#emitDataUpdate(queryParameters, queryId).catch((error: unknown) => {
                        console.error("Error emitting data update after delay:", error);
                    });
                    lastEmitTime = Date.now();
                }, delay);
            }
        };

        watcher.changeStream.on("change", () => {
            emitUpdate().catch((error: unknown) => {
                console.error("Error in emitUpdate:", error);
            });
        });
    }

    /**
     * Emits updated data to clients subscribed to the given queryId.
     *
     * @param queryParameters
     * @param queryId
     */
    async #emitDataUpdate (
        queryParameters: QueryParameters,
        queryId: QueryId
    ) {
        const {query, options} = queryParameters;
        try {
            const data = await this.collection.find(query, options).toArray();
            this.io.to(`${queryId}`).emit("collection::find::update", {queryId, data});
        } catch (error) {
            console.error("Error fetching data for update:", error);
        }
    }
}

export default MongoWatcherCollection;
