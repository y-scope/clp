import type {CompressionMetadata} from "@webui/common/schemas/compress-metadata";
import {RowDataPacket} from "mysql2";

import settings from "../../../../settings.json" with {type: "json"};


enum COMPRESSION_JOBS_TABLE_COLUMN_NAMES {
    ID = "_id",
    STATUS = "status",
    STATUS_MSG = "status_msg",
    START_TIME = "start_time",
    UPDATE_TIME = "update_time",
    DURATION = "duration",
    UNCOMPRESSED_SIZE = "uncompressed_size",
    COMPRESSED_SIZE = "compressed_size",
    CLP_CONFIG = "clp_config",
}

type CompressionMetadataQueryRow = CompressionMetadata & RowDataPacket;

/**
 * Builds the SQL query to fetch recent compression metadata.
 *
 * @return Compression metadata query string.
 */
const getCompressionMetadataQuery = () => `
    SELECT
        id as ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.ID},
        ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS},
        ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS_MSG},
        ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.START_TIME},
        ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UPDATE_TIME},
        ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.DURATION},
        ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE},
        ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.COMPRESSED_SIZE},
        ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.CLP_CONFIG}
    FROM ${settings.SqlDbCompressionJobsTableName}
    WHERE ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UPDATE_TIME} >= NOW() - INTERVAL 30 DAY
    ORDER BY ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.ID} DESC;
`;

export {
    COMPRESSION_JOBS_TABLE_COLUMN_NAMES,
    getCompressionMetadataQuery,
};
export type {CompressionMetadataQueryRow};
