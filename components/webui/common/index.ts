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
interface InterServerEvents {
}

/**
 * Collection associated with each socket connection.
 */
interface SocketData {
    collectionName?: string;
}

/**
 * Enum of search-related signals.
 *
 * This includes request and response signals for various search operations and their respective
 * states.
 */
enum SEARCH_SIGNAL {
    NONE = "none",

    REQ_CANCELLING = "req-cancelling",
    REQ_CLEARING = "req-clearing",
    REQ_QUERYING = "req-querying",

    RESP_DONE = "resp-done",
    RESP_QUERYING = "resp-querying",
}

/**
 * Presto search-related signals.
 */
enum PRESTO_SEARCH_SIGNAL {
    WAITING_FOR_PREREQUISITES = "WAITING_FOR_PREREQUISITES",
    QUEUED = "QUEUED",
    WAITING_FOR_RESOURCES = "WAITING_FOR_RESOURCES",
    DISPATCHING = "DISPATCHING",
    PLANNING = "PLANNING",
    STARTING = "STARTING",
    RUNNING = "RUNNING",
    FINISHING = "FINISHING",
    FINISHED = "FINISHED",
    CANCELED = "CANCELED",
    FAILED = "FAILED",
}

/**
 * CLP query engines.
 */
enum CLP_QUERY_ENGINES {
    CLP = "clp",
    CLP_S = "clp-s",
    PRESTO = "presto",
}

/**
 * MongoDB document for search results metadata. `numTotalResults` is optional
 * since it is only set when the search job is completed.
 */
interface SearchResultsMetadataDocument {
    _id: string;

    // eslint-disable-next-line no-warning-comments
    // TODO: Replace with Nullable<string> when the `@common` directory refactoring is completed.
    errorMsg: string | null;
    errorName: string | null;
    lastSignal: SEARCH_SIGNAL | PRESTO_SEARCH_SIGNAL;
    numTotalResults?: number;
    queryEngine: CLP_QUERY_ENGINES;
}

/**
 * Presto row wrapped in a `row` property to prevent conflicts with MongoDB's `_id` field.
 */
interface PrestoRowObject {
    row: Record<string, unknown>;
}

/**
 * Presto search result in MongoDB.
 */
interface PrestoSearchResult extends PrestoRowObject {
    _id: string;
}

export {
    CLP_QUERY_ENGINES,
    PRESTO_SEARCH_SIGNAL,
    SEARCH_SIGNAL,
};
export type {
    PrestoRowObject,
    PrestoSearchResult,
    SearchResultsMetadataDocument,
    ClientToServerEvents,
    Err,
    InterServerEvents,
    Response,
    ServerToClientEvents,
    SocketData,
    QueryId
};
