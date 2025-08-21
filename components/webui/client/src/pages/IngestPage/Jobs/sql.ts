import {Nullable} from "src/typings/common";

import {settings} from "../../../settings";
import {COMPRESSION_JOBS_TABLE_COLUMN_NAMES} from "../sqlConfig";


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
FROM ${settings.SqlDbCompressionJobsTableName}
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
