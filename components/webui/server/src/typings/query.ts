import {QUERY_JOB_TYPE} from "@webui/common/query";
import {RowDataPacket} from "mysql2/promise";


/**
 * Matching the `QueryJobStatus` class in `job_orchestration.scheduler.constants`.
 */
enum QUERY_JOB_STATUS {
    PENDING = 0,
    RUNNING,
    SUCCEEDED,
    FAILED,
    CANCELLING,
    CANCELLED,
    KILLED,
}

/**
 * List of states that indicate the job is either pending or in progress.
 */
const QUERY_JOB_STATUS_WAITING_STATES = new Set([
    QUERY_JOB_STATUS.PENDING,
    QUERY_JOB_STATUS.RUNNING,
    QUERY_JOB_STATUS.CANCELLING,
]);

/**
 * The `query_jobs` table's column names.
 */
enum QUERY_JOBS_TABLE_COLUMN_NAMES {
    ID = "id",
    TYPE = "type",
    STATUS = "status",
    JOB_CONFIG = "job_config",
}

interface QueryJob extends RowDataPacket {
    [QUERY_JOBS_TABLE_COLUMN_NAMES.ID]: number;
    [QUERY_JOBS_TABLE_COLUMN_NAMES.TYPE]: QUERY_JOB_TYPE;
    [QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS]: QUERY_JOB_STATUS;
    [QUERY_JOBS_TABLE_COLUMN_NAMES.JOB_CONFIG]: string;
}

export type {QueryJob};
export {
    QUERY_JOB_STATUS,
    QUERY_JOB_STATUS_WAITING_STATES,
    QUERY_JOBS_TABLE_COLUMN_NAMES,
};
