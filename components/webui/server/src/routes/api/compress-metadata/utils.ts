import {brotliDecompressSync} from "node:zlib";

import {decode} from "@msgpack/msgpack";
import {Value} from "@sinclair/typebox/value";
import {
    CompressionMetadataDecoded,
    IngestionJob,
} from "@webui/common/schemas/compress-metadata";
import {
    ClpIoConfig,
    ClpIoPartialConfigSchema,
} from "@webui/common/schemas/compression";

import {
    CompressionMetadataQueryRow,
    IngestionJobQueryRow,
} from "./sql.js";


/**
 * Decodes a compressed job config stored in the database.
 *
 * @param jobConfig
 * @return
 * @throws {Error} When the buffer is missing or cannot be decoded.
 */
const decodeJobConfig = (
    jobConfig: unknown
): {clp_config: ClpIoConfig} => {
    if (false === (jobConfig instanceof Buffer)) {
        throw new Error("Missing clp_config buffer for compression metadata");
    }

    try {
        const decodedClpConfig = Value.Parse(
            ClpIoPartialConfigSchema,
            decode(brotliDecompressSync(jobConfig))
        );

        return {clp_config: decodedClpConfig as ClpIoConfig};
    } catch (err: unknown) {
        console.error(err);
        throw new Error("Failed to decode clp_config buffer");
    }
};

/**
 * Maps compression metadata rows from the database to decoded payloads.
 *
 * @param rows
 * @return
 */
const mapCompressionMetadataRows = (
    rows: CompressionMetadataQueryRow[]
): CompressionMetadataDecoded[] => {
    return rows.map(
        ({
            _id,
            clp_config: clpConfig,
            compressed_size: compressedSize,
            duration,
            ingestion_job_id: ingestionJobId,
            s3_bucket: s3Bucket,
            s3_keys: s3Keys,
            start_time: startTime,
            status,
            status_msg: statusMsg,
            uncompressed_size: uncompressedSize,
            update_time: updateTime,
        }): CompressionMetadataDecoded => {
            const {clp_config: decodedClpConfig} = decodeJobConfig(clpConfig);

            const result: CompressionMetadataDecoded = {
                _id: _id,
                clp_config: decodedClpConfig,
                compressed_size: compressedSize,
                duration: duration,
                start_time: startTime,
                status: status,
                status_msg: statusMsg,
                uncompressed_size: uncompressedSize,
                update_time: updateTime,
            };

            if ("undefined" !== typeof ingestionJobId) {
                result.ingestion_job_id = ingestionJobId;
            }

            if (s3Bucket && s3Keys) {
                result.s3_paths = s3Keys.split("|").map((key) => `s3://${s3Bucket}/${key}`);
            }

            return result;
        }
    );
};

/**
 * Maps ingestion job rows from the database to plain objects.
 *
 * @param rows
 * @return
 */
const mapIngestionJobRows = (rows: IngestionJobQueryRow[]): IngestionJob[] => {
    return rows.map(({
        _id,
        config,
        creation_ts: creationTs,
        last_update_ts: lastUpdateTs,
        num_files_compressed: numFilesCompressed,
        status,
    }): IngestionJob => ({
        _id: _id,
        config: config,
        creation_ts: creationTs,
        last_update_ts: lastUpdateTs,
        num_files_compressed: numFilesCompressed,
        status: status,
    }));
};

export {
    decodeJobConfig,
    mapCompressionMetadataRows,
    mapIngestionJobRows,
};
