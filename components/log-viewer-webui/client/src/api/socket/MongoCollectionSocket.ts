import {
    io,
    Socket,
} from "socket.io-client";

import {
    ClientToServerEvents,
    ServerToClientEvents,
    Response,
} from "@common/index.js";

import {MongoCursorSocket} from "./MongoCursorSocket.js";

/**
 * Socket connection to a MongoDB collection residing on a server. Class provides methods to
 * query the collection.
 */
class MongoCollectionSocket {
    private socket: Socket<ServerToClientEvents, ClientToServerEvents>;

    /**
     * Initalizes connection to a MongoDB collection over a socket using a unique namespace.
     * @param collectionName
     */
    constructor (collectionName: string) {
        // eslint-disable-next-line no-warning-comments
        // TODO: use the server URL from the environment / constructor args
        //this.socket = io(`/${collectionName}`);
        this.socket = io();
        this.socket.emit("collection::init", {
            collectionName: collectionName,
        }, (res: Response<void>) => {
            if ("error" in res) {
                console.error("Error initializing collection:", res.error);
            }
        });
        console.log("MongoCollectionSocket initialized for collection:", collectionName);
    }

    /**
     * Selects documents in collection and returns a cursor-like object.
     *
     * @param query
     * @param options
     * @return a MongoCursorSocket.
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
