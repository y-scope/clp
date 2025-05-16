// Maybe move shared stuff to a more common location.

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

/**
 * Checks if the given search signal is a search signal request.
 *
 * @param {SEARCH_SIGNAL} s
 * @return {boolean}
 */
const isSearchSignalReq = (s: SEARCH_SIGNAL): boolean => s.startsWith("req-");

/**
 * Checks if the given search signal is a search signal response.
 *
 * @param {SEARCH_SIGNAL} s
 * @return {boolean}
 */
const isSearchSignalResp = (s: SEARCH_SIGNAL): boolean => s.startsWith("resp-");

/**
 * Checks if the given search signal is a querying request / response.
 *
 * @param {SEARCH_SIGNAL} s
 * @return {boolean}
 */
const isSearchSignalQuerying = (s: SEARCH_SIGNAL): boolean => s.endsWith("-querying");

/**
 * Checks if the given search signal is an operation in progress, which can be used as a
 * condition to disable UI elements.
 *
 * @param {SEARCH_SIGNAL} s
 * @return {boolean}
 */
const isOperationInProgress = (s: SEARCH_SIGNAL): boolean => (
    (true === isSearchSignalReq(s)) ||
    (true === isSearchSignalQuerying(s))
);

/**
 * Enum of Mongo Collection sort orders.
 *
 * @enum {string}
 */
const MONGO_SORT_ORDER = Object.freeze({
    ASCENDING: "asc",
    DESCENDING: "desc",
});

/**
 * Enum of search results cache fields.
 *
 * @enum {string}
 */
const SEARCH_RESULTS_FIELDS = Object.freeze({
    ID: "_id",
    TIMESTAMP: "timestamp",
});

/**
 * The maximum number of results to retrieve for a search.
 */
const SEARCH_MAX_NUM_RESULTS = 1000;

export {
    isOperationInProgress,
    isSearchSignalQuerying,
    isSearchSignalReq,
    isSearchSignalResp,
    MONGO_SORT_ORDER,
    SEARCH_MAX_NUM_RESULTS,
    SEARCH_RESULTS_FIELDS,
    SEARCH_SIGNAL,
};
