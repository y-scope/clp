import {Socket} from "socket.io-client";
import { Nullable } from "../../typings/common";

import {
    ClientToServerEvents,
    ServerToClientEvents,
    Response,
} from "@common/index.js";

/**
 * Represents a cursor-like object recieving documents over a socket connection.
 */
class MongoCursorSocket {
    #socket: Socket<ServerToClientEvents, ClientToServerEvents>;
    #query: object;
    #options: object;
    #queryId: Nullable<number> = null;
    #updateListener: Nullable<(respArgs: {queryId: number; data: object[]}) => void> = null;

    /**
     * @param socket
     * @param query
     * @param options
     */
    constructor (socket: Socket<ServerToClientEvents, ClientToServerEvents>, query: object, options: object) {
        this.#socket = socket;
        this.#query = query;
        this.#options = options;
    }

    /**
     * Subscribes to query watcher for real-time updates.
     *
     * @param onDataUpdate Handler to recieve real-time data updates.
     * @throws Error if subscription fails.
     */
    async subscribe(onDataUpdate: (data: object[]) => void): Promise<void> {

        console.log("Attepting to subscribe to query:", this.#query);

        this.#updateListener = (respArgs: {queryId: number; data: object[]}) => {
            if (this.#queryId === respArgs.queryId) {
                onDataUpdate(respArgs.data);
            }
        };

        this.#socket.on("collection::find::update", this.#updateListener);

        const response: Response<{queryId: number, initialDocuments: object[]}> = await this.#socket.emitWithAck("collection::find::toReactiveArray",
            {
                query: this.#query,
                options: this.#options,
            });

        if ("error" in response) {
            throw new Error(`Subscription failed: ${response.error}`);
        }

        onDataUpdate(response.data.initialDocuments);

        this.#queryId = response.data.queryId;
        console.log("Subscribed to query:", this.#query, "with ID:", this.#queryId);
    }

    /**
     * Unsubscribe from the query watcher.
     */
    unsubscribe(): void {
        if (this.#queryId === null) {
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

        console.log("Unsubscribed from query:", this.#query, "with ID:", this.#queryId);

        this.#queryId = null; // Reset query ID to indicate no active subscription
    }
}

export {MongoCursorSocket};
