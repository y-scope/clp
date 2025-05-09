import {
    ClientToServerEvents,
    ServerToClientEvents,
} from "@common/index.js";
import {
    io,
    Socket,
} from "socket.io-client";

import {MongoCursorSocket} from "./MongoCursorSocket.js";


/**
 * Socket connection to a MongoDB collection residing on a server. Class provides methods to
 * query the collection.
 */
class MongoCollectionSocket {
    private socket: Socket<ServerToClientEvents, ClientToServerEvents>;

    /**
     * Initalizes socket connection to a MongoDB collection on the server.
     *
     * @param collectionName
     */
    constructor (collectionName: string) {
        // eslint-disable-next-line no-warning-comments
        // TODO: Add support for user provided domain name (i.e. io("https://server-domain.com")).
        // Implementation could involve parsing server .env file and moving server .env to a
        // common folder.

        // eslint-disable-next-line no-warning-comments
        // TODO: Current implementation creates a new socket connection for each collection. This
        // could be problematic if the number of collections is large since browsers limit the
        // number of web sockets per domain. An simple change is to use Socket.IO namespace
        // feature, and having Socket.IO multiplex the socket per collection namespace. Another
        // more involved option is to implement a shared socket.
        // Namespace reference: https://socket.io/docs/v4/namespaces/
        this.socket = io();
        this.socket.emit("collection::init", {
            collectionName: collectionName,
        });
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
            this.socket,
            query,
            options,
        );
    }
}


export default MongoCollectionSocket;
