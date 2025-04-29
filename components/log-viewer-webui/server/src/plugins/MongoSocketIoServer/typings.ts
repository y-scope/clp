import {
    ChangeStream,
    Document,
    Filter,
    FindOptions,
} from "mongodb";
import {Socket} from "socket.io";


/**
 * Error response to event.
 */
interface Error {
    error: string;
    queryId?: number;
}

/**
 * Success response to event.
 */
interface Success<T> {
    data: T;
}

/**
 * Event response.
 */
type Response<T> = Error | Success<T>;

/**
 * Events that the client can emit to the server.
 */
type ClientToServerEvents = {
    "disconnect": () => void;
    "collection::init": (
        requestArgs: {
            collectionName: string;
        },
        callback: (res: Response<void>) => void
    ) => void;
    "collection::find::toArray": (
        requestArgs: {
            query: Filter<Document>;
            options: FindOptions;
        },
        callback: (res: Response<{data: Document[]}>) => void) => void;
    "collection::find::toReactiveArray": (
        requestArgs: {
            query: Filter<Document>;
            options: FindOptions;
        },
        callback: (res: Response<{queryId: number}>) => void) => void;
    "collection::find::unsubscribe": (
        requestArgs: {
            queryId: number;
        }
    ) => Promise<void>;
};

/**
 * Events that the server can emit to the client.
 */
interface ServerToClientEvents {
    "collection::find::update": (respArgs: {
        queryId: number;
        data: Document[];
    }) => void;
}

/**
 * Collection associated with each socket connection.
 */
interface SocketData {
    collectionName: string;
}

/**
 * Custom socket type for Mongo Socket IO server.
 */
type MongoCustomSocket = Socket<
    ClientToServerEvents,
    ServerToClientEvents,
    SocketData
>;

/**
 * Unique ID for each active unique query. Multiple clients can subscribe to the same ID if the
 * queries are identical. The ID is also used to represent the socket room, and Mongo change stream.
 */
type QueryId = number;

/**
 * Unique ID to repsent each socket connection.
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
 * Options to connect to MongoDb database.
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
const CLIENT_UPDATE_TIMEOUT_MS = 500;

/**
 * MongoDb change stream for a query, and a list of subscribed connections.
 */
interface Watcher {
    changeStream: ChangeStream;
    subscribers: ConnectionId[];
}

export {
    CLIENT_UPDATE_TIMEOUT_MS,
    ClientToServerEvents,
    ConnectionId,
    DbOptions,
    MongoCustomSocket,
    QueryId,
    QueryParameters,
    Response,
    ServerToClientEvents,
    SocketData,
    Watcher,
};
