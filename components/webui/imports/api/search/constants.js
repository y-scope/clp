/**
 * @typedef {string} SearchSignal
 */

/**
 * Enum of search-related signals.
 *
 * This includes request and response signals for various search operations and their respective
 * states.
 *
 * @enum {SearchSignal}
 */
const SEARCH_SIGNAL = Object.freeze({
    NONE: "none",

    REQ_CANCELLING: "req-cancelling",
    REQ_CLEARING: "req-clearing",
    REQ_QUERYING: "req-querying",

    RESP_DONE: "resp-done",
    RESP_QUERYING: "resp-querying",
});

/**
 * Checks if the given search signal is a search signal request.
 *
 * @param {SearchSignal} s
 * @return {boolean}
 */
const isSearchSignalReq = (s) => s.startsWith("req-");

/**
 * Checks if the given search signal is a search signal response.
 *
 * @param {SearchSignal} s
 * @return {boolean}
 */
const isSearchSignalResp = (s) => s.startsWith("resp-");

/**
 * Checks if the given search signal is a querying request / response.
 *
 * @param {SearchSignal} s
 * @return {boolean}
 */
const isSearchSignalQuerying = (s) => s.endsWith("-querying");

/**
 * Checks if the given search signal is an operation in progress, which can be used as a
 * condition to disable UI elements.
 *
 * @param {SearchSignal} s
 * @return {boolean}
 */
const isOperationInProgress = (s) => (
    (true === isSearchSignalReq(s)) ||
    (true === isSearchSignalQuerying(s))
);

/* eslint-disable sort-keys */
let enumQueryJobStatus;
/**
 * Enum of job statuses, matching the `QueryJobStatus` class in
 * `job_orchestration.query_scheduler.constants`.
 *
 * @enum {number}
 */
const QUERY_JOB_STATUS = Object.freeze({
    PENDING: (enumQueryJobStatus = 0),
    RUNNING: ++enumQueryJobStatus,
    SUCCEEDED: ++enumQueryJobStatus,
    FAILED: ++enumQueryJobStatus,
    CANCELLING: ++enumQueryJobStatus,
    CANCELLED: ++enumQueryJobStatus,
});
/* eslint-enable sort-keys */

const QUERY_JOB_STATUS_WAITING_STATES = [
    QUERY_JOB_STATUS.PENDING,
    QUERY_JOB_STATUS.RUNNING,
    QUERY_JOB_STATUS.CANCELLING,
];

/* eslint-disable sort-keys */
let enumQueryType;
/**
 * Enum of job type, matching the `QueryJobType` class in
 * `job_orchestration.query_scheduler.constants`.
 *
 * @enum {number}
 */
const QUERY_JOB_TYPE = Object.freeze({
    SEARCH_OR_AGGREGATION: (enumQueryType = 0),
    EXTRACT_IR: ++enumQueryType,
});
/* eslint-enable sort-keys */

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
    QUERY_JOB_STATUS,
    QUERY_JOB_STATUS_WAITING_STATES,
    QUERY_JOB_TYPE,
    SEARCH_MAX_NUM_RESULTS,
    SEARCH_RESULTS_FIELDS,
    SEARCH_SIGNAL,
};
