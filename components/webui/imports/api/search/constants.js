let enumSearchSignal;
/**
 * Enum of search-related signals.
 *
 * This includes request and response signals for various search operations and their respective
 * states.
 *
 * @constant
 * @type {Object}
 */
export const SearchSignal = Object.freeze({
    NONE: (enumSearchSignal=0),

    REQ_MASK: (enumSearchSignal = 0x10000000),
    REQ_CLEARING: ++enumSearchSignal,
    REQ_CANCELLING: ++enumSearchSignal,
    REQ_QUERYING: ++enumSearchSignal,

    RSP_MASK: (enumSearchSignal = 0x20000000),
    RSP_DONE: ++enumSearchSignal,
    RSP_ERROR: ++enumSearchSignal,
    RSP_QUERYING: ++enumSearchSignal,
});

export const isSearchSignalReq = (s) => (0 !== (SearchSignal.REQ_MASK & s));
export const isSearchSignalRsp = (s) => (0 !== (SearchSignal.RSP_MASK & s));
export const isSearchSignalQuerying = (s) => (
    [SearchSignal.REQ_QUERYING, SearchSignal.RSP_QUERYING].includes(s)
);

let enumJobStatus;
/**
 * Enum of job statuses, matching the `JobStatus` class in `job_orchestration.search_scheduler.common`.
 *
 * @constant
 * @type {Object}
 */
export const JobStatus = Object.freeze({
    PENDING: (enumJobStatus=0),
    RUNNING: ++enumJobStatus,
    SUCCESS: ++enumJobStatus,
    FAILED: ++enumJobStatus,
    CANCELLING: ++enumJobStatus,
    CANCELLED: ++enumJobStatus
})

export const JOB_STATUS_WAITING_STATES = [
    JobStatus.PENDING,
    JobStatus.RUNNING,
    JobStatus.CANCELLING
]

export const INVALID_JOB_ID = -1;
