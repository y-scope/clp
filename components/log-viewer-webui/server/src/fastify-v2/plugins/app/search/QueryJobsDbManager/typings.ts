import {Nullable} from "../../../../../typings/common.js";

/**
 * Interval in milliseconds for polling the completion status of a job.
 */
export const JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS = 500;

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
 * MongoDB document for search results metadata. `numTotalResults` is optional
 * since it is only set when the search job is completed.
 */
interface SearchResultsMetadataDocument {
    _id: string;
    errorMsg: Nullable<string>;
    lastSignal: SEARCH_SIGNAL;
    numTotalResults?: number;
}

export type {SearchResultsMetadataDocument};
export {SEARCH_SIGNAL};
