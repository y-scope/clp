import {
    type CompressionMetadata,
    IngestionJobStatus,
} from "@webui/common/schemas/compress-metadata";
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

/**
 * Table names are hardcoded constants defined by the log-ingestor.
 */
const INGESTION_JOB_TABLE_NAME = "ingestion_job";
const INGESTED_S3_OBJECT_METADATA_TABLE_NAME = "ingested_s3_object_metadata";

type CompressionMetadataQueryRow = CompressionMetadata & {
    ingestion_job_id?: number | null;
    s3_bucket?: string | null;
    s3_keys?: string | null;
} & RowDataPacket;

interface IngestionJobQueryRow extends RowDataPacket {
    _id: number;
    config: string;
    creation_ts: string;
    last_update_ts: string;
    num_files_compressed: number;
    status: IngestionJobStatus;
}

const COMPRESSION_METADATA_QUERY_LIMIT = 1000;

/**
 * Builds the SQL query to fetch recent compression metadata, joining with
 * ingested_s3_object_metadata to get the associated ingestion_job_id (if any).
 *
 * @return Compression metadata query string.
 */
const getCompressionMetadataQuery = () => `
    SELECT
        cj.id as ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.ID},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS_MSG},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.START_TIME},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UPDATE_TIME},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.DURATION},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.COMPRESSED_SIZE},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.CLP_CONFIG},
        MAX(isom.ingestion_job_id) as ingestion_job_id,
        MAX(isom.bucket) as s3_bucket,
        GROUP_CONCAT(isom.\`key\` SEPARATOR '|') as s3_keys
    FROM ${settings.SqlDbCompressionJobsTableName} cj
    LEFT JOIN ${INGESTED_S3_OBJECT_METADATA_TABLE_NAME} isom ON isom.compression_job_id = cj.id
    GROUP BY
        cj.id,
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS_MSG},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.START_TIME},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UPDATE_TIME},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.DURATION},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.COMPRESSED_SIZE},
        cj.${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.CLP_CONFIG}
    ORDER BY cj.id DESC
    LIMIT ${COMPRESSION_METADATA_QUERY_LIMIT};
`;

/**
 * Builds the SQL query to fetch recent ingestion jobs.
 *
 * @return Ingestion jobs query string.
 */
const getIngestionJobsQuery = () => `
    SELECT
        id as _id,
        config,
        status,
        num_files_compressed,
        creation_ts,
        last_update_ts
    FROM ${INGESTION_JOB_TABLE_NAME}
    ORDER BY id DESC
    LIMIT ${COMPRESSION_METADATA_QUERY_LIMIT};
`;

export {
    COMPRESSION_JOBS_TABLE_COLUMN_NAMES,
    getCompressionMetadataQuery,
    getIngestionJobsQuery,
};
export type {
    CompressionMetadataQueryRow,
    IngestionJobQueryRow,
};
