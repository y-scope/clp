import {CLP_QUERY_ENGINES} from "./config";


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

export {
    PRESTO_SEARCH_SIGNAL,
    SEARCH_SIGNAL,
};
export type {SearchResultsMetadataDocument};
