import {CLP_QUERY_ENGINES} from "./config";
import {Nullable} from "./utility-types";


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
    QUERYING = "QUERYING",
    DONE = "DONE",
    FAILED = "FAILED",
}


/**
 * MongoDB document for search results metadata. `numTotalResults` is optional
 * since it is only set when the search job is completed.
 */
interface SearchResultsMetadataDocument {
    _id: string;
    errorMsg: Nullable<string>;
    errorName: Nullable<string>;
    lastSignal: SEARCH_SIGNAL | PRESTO_SEARCH_SIGNAL;
    numTotalResults?: number;
    queryEngine: CLP_QUERY_ENGINES;
}

export {
    PRESTO_SEARCH_SIGNAL,
    SEARCH_SIGNAL,
};
export type {SearchResultsMetadataDocument};
