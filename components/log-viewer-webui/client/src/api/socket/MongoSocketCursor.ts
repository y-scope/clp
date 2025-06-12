import {
    ClientToServerEvents,
    QueryId,
    Response,
    ServerToClientEvents,
} from "@common/index.js";
import {Socket} from "socket.io-client";

import {Nullable} from "../../typings/common";


/**
 * A cursor-like object receiving MongoDB documents over a socket connection.
 */
class MongoSocketCursor {
    #socket: Socket<ServerToClientEvents, ClientToServerEvents>;

    #collectionName: string;

    #query: object;

    #options: object;

    #queryId: Nullable<QueryId> = null;


    // Listener for data updates from the server.
    #updateListener: Nullable<(respArgs: {queryId: number; data: object[]}) => void> = null;

    /**
     * @param socket
     * @param collectionName
     * @param query
     * @param options
     */
    constructor (
        socket: Socket<ServerToClientEvents, ClientToServerEvents>,
        collectionName: string,
        query: object,
        options: object
    ) {
        this.#socket = socket;
        this.#collectionName = collectionName;
        this.#query = query;
        this.#options = options;
    }

    /**
     * Subscribes to query watcher for real-time updates.
     *
     * @param onDataUpdate Handler which sets data updates from the server in react ui component.
     * @throws {Error} if subscription fails.
     */
    async subscribe (onDataUpdate: (data: object[]) => void): Promise<void> {
        console.debug(
            `Subscribing to query: ${JSON.stringify(this.#query)} ` +
            `with options:${JSON.stringify(this.#options)} ` +
            `on collection:${this.#collectionName}`
        );

        this.#updateListener = (respArgs: {queryId: number; data: object[]}) => {
            // Server sends updates for multiple queryIDs using the same event name.
            if (this.#queryId === respArgs.queryId) {
                onDataUpdate(respArgs.data);
            }
        };

        this.#socket.on("collection::find::update", this.#updateListener);

        const response: Response<{queryId: number; initialDocuments: object[]}> =
            await this.#socket.emitWithAck(
                "collection::find::subscribe",
                {
                    collectionName: this.#collectionName,
                    query: this.#query,
                    options: this.#options,
                }
            );

        if ("error" in response) {
            this.#socket.off("collection::find::update", this.#updateListener);
            throw new Error(`Subscription failed: ${response.error}`);
        }

        // Set the initial documents received from the server.
        onDataUpdate(response.data.initialDocuments);

        this.#queryId = response.data.queryId;
        console.debug(
            `Successfully subscribed to query: ${JSON.stringify(this.#query)} ` +
            `with options:${JSON.stringify(this.#options)} ` +
            `on collection:${this.#collectionName} ` +
            `MongoSocketIoQueryID:${this.#queryId}`
        );
    }

    /**
     * Unsubscribe from the query.
     */
    unsubscribe (): void {
        if (null === this.#queryId) {
            console.error("Attempted to unsubscribe, but no active subscription exists.");

            return;
        }


        this.#socket.emit("collection::find::unsubscribe", {
            queryId: this.#queryId,
        });

        if (this.#updateListener) {
            this.#socket.off("collection::find::update", this.#updateListener);
            this.#updateListener = null;
        }

        console.debug(`Unsubscribed from MongoSocketIoQueryID:${this.#queryId}.`);

        this.#queryId = null;
    }
}

export {MongoSocketCursor};
