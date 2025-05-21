import {Nullable} from "src/typings/common";

import {SQL_CONFIG} from "../sqlConfig";


/**
 * Enum of the column names for the `compression_jobs` table.
 *
 * @enum {string}
 */
const COMPRESSION_JOBS_TABLE_COLUMN_NAMES = Object.freeze({
    CLP_BINARY_VERSION: "clp_binary_version",
    CLP_CONFIG: "clp_config",
    COMPRESSED_SIZE: "compressed_size",
    CREATION_TIME: "creation_time",
    DURATION: "duration",
    ID: "id",
    NUM_TASKS: "num_tasks",
    NUM_TASKS_COMPLETED: "num_tasks_completed",
    ORIGINAL_SIZE: "original_size",
    START_TIME: "start_time",
    STATUS: "status",
    STATUS_MSG: "status_msg",
    UNCOMPRESSED_SIZE: "uncompressed_size",
    UPDATE_TIME: "update_time",
});

/**
 * Builds the query string to query jobs.
 *
 * @param lastUpdateTimestampSeconds
 * @return
 */
const getQueryJobsSql = (lastUpdateTimestampSeconds: number) => `
SELECT
    UNIX_TIMESTAMP() as retrieval_time,
    id as _id,
    ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS},
    ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS_MSG},
    ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.START_TIME},
    ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UPDATE_TIME},
    ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.DURATION},
    ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE},
    ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.COMPRESSED_SIZE}
FROM ${SQL_CONFIG.SqlDbCompressionJobsTableName}
WHERE ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UPDATE_TIME} >= 
    FROM_UNIXTIME(${lastUpdateTimestampSeconds}) - 1
ORDER BY _id DESC;`;

interface QueryJobsItem {
    compressed_size: number;
    duration: Nullable<number>;
    retrieval_time: number;
    start_time: Nullable<string>;
    status: number;
    status_msg: string;
    uncompressed_size: number;
    update_time: string;
    _id: number;
}

type QueryJobsResp = QueryJobsItem[];

export type {
    QueryJobsItem, QueryJobsResp,
};
export {getQueryJobsSql};
