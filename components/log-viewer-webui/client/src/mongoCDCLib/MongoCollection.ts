import {
    io,
    Socket,
} from "socket.io-client";

import {
    MongoCollectionReactiveCursor,
    type ServerError,
} from "./MongoCollectionReactiveCursor.js";


let sharedSocket: Socket | null = null;

/**
 * Instantiate a shared socket connection to the server.
 *
 * @return The shared socket connection.
 */
const getSharedSocket = (): Socket => {
    if (!sharedSocket) {
        // You can pass a URL here if needed (e.g., from environment vars)
        sharedSocket = io();
    }

    return sharedSocket;
};

/**
 * Represents a MongoDB collection that can be queried over a socket connection.
 *
 * @class MongoCollection
 */
class MongoCollection {
    private socket: Socket;

    /**
     * Creates an instance of MongoCollection.
     *
     * @param collectionName The name of the collection to interact with.
     */
    constructor (collectionName: string) {
        // eslint-disable-next-line no-warning-comments
        // TODO: use the server URL from the environment / constructor args
        this.socket = getSharedSocket();

        this.socket.emit("collection::init", {
            collectionName: collectionName,
        }, (response: ServerError) => {
            if ("error" in response && "collectionName" in response) {
                if (collectionName === response.collectionName) {
                    console.error("Error initializing collection:", response.error);
                }
            }
        });
    }

    // eslint-disable-next-line no-warning-comments
    // TODO: add support for non-reactive cursors
    /**
     * Finds documents in the collection based on the provided query and options.
     *
     * @param query The query object to filter results.
     * @param options The options for the query (e.g., sort, limit).
     * @return An instance of MongoCollectionReactiveCursor for reactive querying.
     */
    find (query: object, options: object) {
        return new MongoCollectionReactiveCursor({
            options: options,
            query: query,
            socket: this.socket,
        });
    }
}


export default MongoCollection;
