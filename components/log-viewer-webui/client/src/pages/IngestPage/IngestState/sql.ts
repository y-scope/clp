/**
 * Sql table names.
 */
const CONFIG = Object.freeze({
    SqlDbClpArchivesTableName: "clp_archives",
    SqlDbClpFilesTableName: "clp_files",
    SqlDbCompressionJobsTableName: "compression_jobs",
    SqlDbQueryJobsTableName: "query_jobs",
});

/**
 * Enum of the column names for the `clp_archives` table.
 *
 * @enum {string}
 */
const CLP_ARCHIVES_TABLE_COLUMN_NAMES = Object.freeze({
    BEGIN_TIMESTAMP: "begin_timestamp",
    END_TIMESTAMP: "end_timestamp",
    UNCOMPRESSED_SIZE: "uncompressed_size",
    SIZE: "size",
});

/**
 * Enum of the column names for the `clp_files` table.
 *
 * @enum {string}
 */
const CLP_FILES_TABLE_COLUMN_NAMES = Object.freeze({
    ORIG_FILE_ID: "orig_file_id",
    NUM_MESSAGES: "num_messages",
});

/**
 * Builds the query string to query stats.
 *
 * @return
 */
const getQueryStatsSql = () => `
SELECT
    a.begin_timestamp         AS begin_timestamp,
    a.end_timestamp           AS end_timestamp,
    a.total_uncompressed_size AS total_uncompressed_size,
    a.total_compressed_size   AS total_compressed_size,
    b.num_files               AS num_files,
    b.num_messages            AS num_messages
FROM
(
    SELECT
        MIN(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.BEGIN_TIMESTAMP})   AS begin_timestamp,
        MAX(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP})     AS end_timestamp,
        SUM(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE}) AS total_uncompressed_size,
        SUM(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.SIZE})              AS total_compressed_size
    FROM ${CONFIG.SqlDbClpArchivesTableName}
) a,
(
    SELECT
        NULLIF(COUNT(DISTINCT ${CLP_FILES_TABLE_COLUMN_NAMES.ORIG_FILE_ID}), 0) AS num_files,
        SUM(${CLP_FILES_TABLE_COLUMN_NAMES.NUM_MESSAGES})                       AS num_messages
    FROM ${CONFIG.SqlDbClpFilesTableName}
) b;
`;

interface QueryStatsItem {
    begin_timestamp: number;
    end_timestamp: number;
    total_uncompressed_size: number;
    total_compressed_size: number;
    num_files: number;
    num_messages: number;
}

type QueryStatsResp = QueryStatsItem[];

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
FROM ${CONFIG.SqlDbCompressionJobsTableName}
WHERE ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UPDATE_TIME} >= 
    FROM_UNIXTIME(${lastUpdateTimestampSeconds}) - 1
ORDER BY _id DESC;`;

export type {
    QueryStatsItem, QueryStatsResp,
};
export {
    getQueryJobsSql, getQueryStatsSql,
};
