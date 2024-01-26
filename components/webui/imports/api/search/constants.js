let enumSearchSignal;
export const SearchSignal = Object.freeze({
    NONE: (enumSearchSignal=0),
    REQ_MASK: (enumSearchSignal = 0x10000000),
    REQ_CANCELLING: ++enumSearchSignal,
    REQ_QUERYING: ++enumSearchSignal,
    REQ_CLEARING: ++enumSearchSignal,
    RSP_MASK: (enumSearchSignal = 0x20000000),
    RSP_DONE: ++enumSearchSignal,
    RSP_ERROR: ++enumSearchSignal,
    RSP_SEARCHING: ++enumSearchSignal,
});
export const isSearchSignalReq = (e) => (0 !== (SearchSignal.REQ_MASK & e));
export const isSearchSignalRsp = (e) => (0 !== (SearchSignal.RSP_MASK & e));

// below should match `job_orchestration.search_scheduler.common`: class JobStatus
let enumJobStatus;
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
