/* eslint-disable sort-keys */
const COMPRESSION_JOBS_TABLE_COLUMN_NAMES = Object.freeze({
    ID: "id",
    STATUS: "status",
    STATUS_MSG: "status_msg",
    CREATION_TIME: "creation_time",
    START_TIME: "start_time",
    DURATION: "duration",
    ORIGINAL_SIZE: "original_size",
    UNCOMPRESSED_SIZE: "uncompressed_size",
    COMPRESSED_SIZE: "compressed_size",
    NUM_TASKS: "num_tasks",
    NUM_TASKS_COMPLETED: "num_tasks_completed",
    CLP_BINARY_VERSION: "clp_binary_version",
    CLP_CONFIG: "clp_config",
});
/* eslint-enable sort-keys */

let enumCompressionJobStatus;
/**
 * Enum of compression job statuses, matching the `CompressionJobStatus` class in
 * `job_orchestration.scheduler.constants`.
 *
 * @enum {string}
 */
const COMPRESSION_JOB_STATUS = Object.freeze({
    PENDING: (enumCompressionJobStatus = 0),
    RUNNING: ++enumCompressionJobStatus,
    SUCCEEDED: ++enumCompressionJobStatus,
    FAILED: ++enumCompressionJobStatus,
});

/**
 * List of waiting states for a compression job.
 * @see COMPRESSION_JOB_STATUS
 */
const COMPRESSION_JOB_WAITING_STATES = Object.freeze([
    COMPRESSION_JOB_STATUS.PENDING,
    COMPRESSION_JOB_STATUS.RUNNING,
]);

/**
 * Represents the possible status names for a compression job.
 * @type {ReadonlyArray<string>}
 */
const COMPRESSION_JOB_STATUS_NAMES = Object.freeze([
    "PENDING",
    "RUNNING",
    "SUCCEEDED",
    "FAILED",
]);

/**
 * The maximum number of compression jobs that is retrieved at a time.
 */
const COMPRESSION_MAX_RETRIEVE_JOBS = 5;

export {
    COMPRESSION_JOB_STATUS,
    COMPRESSION_JOB_STATUS_NAMES,
    COMPRESSION_JOB_WAITING_STATES,
    COMPRESSION_JOBS_TABLE_COLUMN_NAMES,
    COMPRESSION_MAX_RETRIEVE_JOBS,
};
