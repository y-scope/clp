let enumSearchSignal;
/**
 * Enum of search-related signals.
 *
 * This includes request and response signals for various search operations and their respective
 * states.
 *
 * @type {Object}
 */
const SEARCH_SIGNAL = Object.freeze({
    NONE: (enumSearchSignal = 0),

    REQ_MASK: (enumSearchSignal = 0x10000000),
    REQ_CLEARING: ++enumSearchSignal,
    REQ_CANCELLING: ++enumSearchSignal,
    REQ_QUERYING: ++enumSearchSignal,

    RESP_MASK: (enumSearchSignal = 0x20000000),
    RESP_DONE: ++enumSearchSignal,
    RESP_QUERYING: ++enumSearchSignal,
});

const isSearchSignalReq = (s) => (0 !== (SEARCH_SIGNAL.REQ_MASK & s));
const isSearchSignalResp = (s) => (0 !== (SEARCH_SIGNAL.RESP_MASK & s));
const isSearchSignalQuerying = (s) => (
    [
        SEARCH_SIGNAL.REQ_QUERYING,
        SEARCH_SIGNAL.RESP_QUERYING,
    ].includes(s)
);
const isOperationInProgress = (s) => (
    (true === isSearchSignalReq(s)) ||
    (true === isSearchSignalQuerying(s))
);

/* eslint-disable sort-keys */
let enumSearchJobStatus;
/**
 * Enum of job statuses, matching the `SearchJobStatus` class in
 * `job_orchestration.search_scheduler.constants`.
 *
 * @readonly
 * @enum {number}
 */
const SEARCH_JOB_STATUS = Object.freeze({
    PENDING: (enumSearchJobStatus = 0),
    RUNNING: ++enumSearchJobStatus,
    SUCCEEDED: ++enumSearchJobStatus,
    FAILED: ++enumSearchJobStatus,
    CANCELLING: ++enumSearchJobStatus,
    CANCELLED: ++enumSearchJobStatus,
});
/* eslint-enable sort-keys */


const JOB_STATUS_WAITING_STATES = [
    SEARCH_JOB_STATUS.PENDING,
    SEARCH_JOB_STATUS.RUNNING,
    SEARCH_JOB_STATUS.CANCELLING,
];

/**
 * Enum of Mongo Collection sort orders.
 *
 * @readonly
 * @enum {string}
 */
const MONGO_SORT_ORDER = Object.freeze({
    ASCENDING: "asc",
    DESCENDING: "desc",
});

/**
 * Enum of search results cache fields.
 *
 * @readonly
 * @enum {string}
 */
const SEARCH_RESULTS_FIELDS = Object.freeze({
    ID: "_id",
    TIMESTAMP: "timestamp",
});


const INVALID_JOB_ID = -1;

/**
 * The maximum number of results to retrieve for a search.
 */
const SEARCH_MAX_NUM_RESULTS = 1000;

export {
    INVALID_JOB_ID,
    isOperationInProgress,
    isSearchSignalQuerying,
    isSearchSignalReq,
    isSearchSignalResp,
    JOB_STATUS_WAITING_STATES,
    MONGO_SORT_ORDER,
    SEARCH_JOB_STATUS,
    SEARCH_MAX_NUM_RESULTS,
    SEARCH_RESULTS_FIELDS,
    SEARCH_SIGNAL,
};
