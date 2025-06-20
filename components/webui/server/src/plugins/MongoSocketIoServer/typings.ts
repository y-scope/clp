import {
    ChangeStream,
    Document,
    Filter,
    FindOptions,
} from "mongodb";
import {Socket} from "socket.io";

import {
    ClientToServerEvents,
    InterServerEvents,
    ServerToClientEvents,
    SocketData,
} from "../../../../common/index.js";


/**
 * Custom socket type for Mongo Socket IO server.
 */
type MongoCustomSocket = Socket<
    ClientToServerEvents,
    ServerToClientEvents,
    InterServerEvents,
    SocketData
>;

/**
 * Unique ID to represent each socket connection.
 */
type ConnectionId = string;


/**
 * Parameters for MongoDB queries.
 */
interface QueryParameters {
    collectionName: string;
    query: Filter<Document>;
    options: FindOptions;
}

/**
 * Options to connect to MongoDB database.
 */
interface DbOptions {
    // Name of database.
    database: string;
    host: string;
    port: number;
}

/**
 * Timeout for emitting updates to the client.
 */
const CLIENT_UPDATE_TIMEOUT_MILLIS = 500;

/**
 * MongoDB change stream for a query, and a list of subscribed connections. Subscribed connections
 * can include duplicates if the same connection subscribes to the same query multiple times.
 */
interface Watcher {
    changeStream: ChangeStream;
    subscribers: ConnectionId[];
}

export {
    CLIENT_UPDATE_TIMEOUT_MILLIS,
    ConnectionId,
    DbOptions,
    MongoCustomSocket,
    QueryParameters,
    Watcher,
};
