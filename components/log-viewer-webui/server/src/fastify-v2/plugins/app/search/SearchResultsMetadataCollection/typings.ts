import { Nullable } from "../../../../../typings/common.js";

interface SearchResultsMetadataDocument {
    _id: string;
    errorMsg: Nullable<string>;
    lastSignal: SEARCH_SIGNAL;
    numTotalResults?: number;
}

/**
 * Enum of search-related signals.
 *
 * This includes request and response signals for various search operations and their respective
 * states.
 *
 * @enum {string}
 */
enum SEARCH_SIGNAL {
    NONE = "none",

    REQ_CANCELLING = "req-cancelling",
    REQ_CLEARING = "req-clearing",
    REQ_QUERYING = "req-querying",

    RESP_DONE = "resp-done",
    RESP_QUERYING = "resp-querying",
}



export { SearchResultsMetadataDocument, SEARCH_SIGNAL };
