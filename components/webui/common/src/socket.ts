/**
 * Unique ID for each active unique query. Multiple clients can subscribe to the same ID if the
 * queries are identical. The ID is also used to represent the socket room, and MongoDB
 * change stream.
 */
type QueryId = number;

/**
 * Error response to event.
 */
interface Err {
    error: string;
    queryId?: QueryId;
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
type Response<T> = Err | Success<T>;


/**
 * Events that the client can emit to the server.
 */
type ClientToServerEvents = {
    "disconnect": () => void;
    "collection::find::subscribe": (
        requestArgs: {
            collectionName: string;
            query: object;
            options: object;
        },
        callback: (res: Response<{queryId: QueryId; initialDocuments: object[]}>) => void) => void;
    "collection::find::unsubscribe": (
        requestArgs: {
            queryId: QueryId;
        }
    ) => Promise<void>;
};

/**
 * Events that the server can emit to the client.
 */
interface ServerToClientEvents {
    // eslint-disable-next-line no-warning-comments
    // TODO: Consider replacing this with `collection::find::update${number}`, which will
    // limit callbacks being triggered in the client to their respective query IDs.
    "collection::find::update": (respArgs: {
        queryId: QueryId;
        data: object[];
    }) => void;
}

/**
 * Empty but required by Socket IO.
 */
// eslint-disable-next-line @typescript-eslint/no-empty-object-type
interface InterServerEvents {
}

/**
 * Collection associated with each socket connection.
 */
interface SocketData {
    collectionName?: string;
}

export type {
    ClientToServerEvents,
    Err,
    InterServerEvents,
    QueryId,
    Response,
    ServerToClientEvents,
    SocketData,
};