/**
 * Matching the `CompressionJobStatus` class in `job_orchestration.scheduler.constants`.
 *
 * @enum {number}
 */
enum COMPRESSION_JOB_STATUS {
    PENDING = 0,
    RUNNING,
    SUCCEEDED,
    FAILED,
    KILLED,
}

/**
 * The `compression_jobs` table's column names.
 *
 * @enum {string}
 */
enum COMPRESSION_JOBS_TABLE_COLUMN_NAMES {
    ID = "id",
    STATUS = "status",
    JOB_CONFIG = "clp_config",
}


export {
    COMPRESSION_JOB_STATUS,
    COMPRESSION_JOBS_TABLE_COLUMN_NAMES,
};
