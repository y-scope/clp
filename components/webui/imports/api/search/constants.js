let enumSearchSignal;
/**
 * Enum of search-related signals.
 *
 * This includes request and response signals for various search operations and their respective
 * states.
 *
 * @type {Object}
 */
const SearchSignal = Object.freeze({
    NONE: (enumSearchSignal = 0),

    REQ_MASK: (enumSearchSignal = 0x10000000),
    REQ_CLEARING: ++enumSearchSignal,
    REQ_CANCELLING: ++enumSearchSignal,
    REQ_QUERYING: ++enumSearchSignal,

    RESP_MASK: (enumSearchSignal = 0x20000000),
    RESP_DONE: ++enumSearchSignal,
    RESP_QUERYING: ++enumSearchSignal,
});

const isSearchSignalReq = (s) => (0 !== (SearchSignal.REQ_MASK & s));
const isSearchSignalResp = (s) => (0 !== (SearchSignal.RESP_MASK & s));
const isSearchSignalQuerying = (s) => (
    [
        SearchSignal.REQ_QUERYING,
        SearchSignal.RESP_QUERYING,
    ].includes(s)
);

let enumJobStatus;
/**
 * Enum of job statuses, matching the `SearchJobStatus` class in
 * `job_orchestration.search_scheduler.constants`.
 *
 * @type {Object}
 */
const JobStatus = Object.freeze({
    PENDING: (enumJobStatus = 0),
    RUNNING: ++enumJobStatus,
    SUCCESS: ++enumJobStatus,
    FAILED: ++enumJobStatus,
    CANCELLING: ++enumJobStatus,
    CANCELLED: ++enumJobStatus,
});

const JOB_STATUS_WAITING_STATES = [
    JobStatus.PENDING,
    JobStatus.RUNNING,
    JobStatus.CANCELLING,
];

const INVALID_JOB_ID = -1;

export {
    SearchSignal,
    isSearchSignalReq,
    isSearchSignalResp,
    isSearchSignalQuerying,
    JobStatus,
    JOB_STATUS_WAITING_STATES,
    INVALID_JOB_ID,
};
