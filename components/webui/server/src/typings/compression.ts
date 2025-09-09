import {RowDataPacket} from "mysql2/promise";


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
 * List of states that indicate the job is either pending or in progress.
 */
const COMPRESSION_JOB_STATUS_WAITING_STATES = new Set([
    COMPRESSION_JOB_STATUS.PENDING,
    COMPRESSION_JOB_STATUS.RUNNING,
]);

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

interface CompressionJob extends RowDataPacket {
    [COMPRESSION_JOBS_TABLE_COLUMN_NAMES.ID]: number;
    [COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS]: COMPRESSION_JOB_STATUS;
    [COMPRESSION_JOBS_TABLE_COLUMN_NAMES.JOB_CONFIG]: string;
}

export type {CompressionJob};
export {
    COMPRESSION_JOB_STATUS,
    COMPRESSION_JOB_STATUS_WAITING_STATES,
    COMPRESSION_JOBS_TABLE_COLUMN_NAMES,
};
