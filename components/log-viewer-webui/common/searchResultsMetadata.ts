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
 * Checks if the given search signal is a search signal request.
 *
 * @param {SearchSignal} s
 * @return {boolean}
 */
const isSearchSignalReq = (s: SEARCH_SIGNAL) => s.startsWith("req-");

/**
 * Checks if the given search signal is a search signal response.
 *
 * @param {SearchSignal} s
 * @return {boolean}
 */
const isSearchSignalResp = (s: SEARCH_SIGNAL) => s.startsWith("resp-");

/**
 * Checks if the given search signal is a querying request / response.
 *
 * @param {SearchSignal} s
 * @return {boolean}
 */
const isSearchSignalQuerying = (s: SEARCH_SIGNAL) => s.endsWith("-querying");

/**
 * Checks if the given search signal is an operation in progress, which can be used as a
 * condition to disable UI elements.
 *
 * @param {SearchSignal} s
 * @return {boolean}
 */
const isOperationInProgress = (s: SEARCH_SIGNAL) => (
    (true === isSearchSignalReq(s)) ||
    (true === isSearchSignalQuerying(s))
);

/**
 * MongoDB document for search results metadata. `numTotalResults` is optional
 * since it is only set when the search job is completed.
 */
interface SearchResultsMetadataDocument {
    _id: string;
    errorMsg: string | null;
    lastSignal: SEARCH_SIGNAL;
    numTotalResults?: number;
}

export type {SearchResultsMetadataDocument};
export {
    isSearchSignalQuerying,
    isSearchSignalResp,
    isOperationInProgress,
    SEARCH_SIGNAL
};
