import {
    ClientToServerEvents,
    ServerToClientEvents,
} from "@common/index.js";
import {Socket} from "socket.io-client";

import {MongoCursorSocket} from "./MongoCursorSocket.js";
import {getSharedSocket} from "./SocketSingleton.js";


/**
 * Socket connection to a MongoDB collection residing on a server. Class provides methods to
 * query the collection.
 */
class MongoCollectionSocket {
    collectionName: string;

    #socket: Socket<ServerToClientEvents, ClientToServerEvents>;

    /**
     * Initalizes socket connection to a MongoDB collection on the server.
     *
     * @param collectionName
     */
    constructor (collectionName: string) {
        this.#socket = getSharedSocket();
        this.collectionName = collectionName;
        console.log(`MongoDB collection:${collectionName} initialized.`);
    }

    /**
     * Selects documents in collection and returns a cursor-like object.
     *
     * @param query
     * @param options
     * @return a `MongoCursorSocket`.
     */

    find (query: object, options: object) {
        return new MongoCursorSocket(
            this.#socket,
            this.collectionName,
            query,
            options,
        );
    }
}


export default MongoCollectionSocket;
