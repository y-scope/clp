/**
 * Error response to event.
 */
interface Err {
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
type Response<T> = Err | Success<T>;

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
    "collection::find::toReactiveArray": (
        requestArgs: {
            query: object;
            options: object;
        },
        callback: (res: Response<{queryId: number; initialDocuments: object[]}>) => void) => void;
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
        data: object[];
    }) => void;
}

/**
 * Empty but required by Socket IO.
 */
interface InterServerEvents {
}

/**
 * Collection associated with each socket connection.
 */
interface SocketData {
    collectionName: string;
}

export {
    ClientToServerEvents,
    Err,
    InterServerEvents,
    Response,
    ServerToClientEvents,
    SocketData,
};
