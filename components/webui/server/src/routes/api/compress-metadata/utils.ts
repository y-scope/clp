import {decode} from "@msgpack/msgpack";
import {ClpIoConfig} from "@webui/common/schemas/compression";
import {CompressionMetadataDecoded} from "@webui/common/schemas/compress-metadata";
import {brotliDecompressSync} from "node:zlib";

import {CompressionMetadataQueryRow} from "./sql.js";

/**
 * Decodes a compressed job config stored in the database.
 *
 * @param jobConfig
 * @return
 */
export const decodeJobConfig = (
    jobConfig: unknown
): {clp_config: ClpIoConfig} => {
    if (!(jobConfig instanceof Buffer)) {
        throw new Error("Missing clp_config buffer for compression metadata");
    }

    try {
        const clpConfig = decode(brotliDecompressSync(jobConfig)) as ClpIoConfig;

        return {clp_config: clpConfig};
    } catch (err: unknown) {
        // eslint-disable-next-line no-console
        console.error(err);
        throw new Error("Failed to decode clp_config buffer");
    }
};

/**
 * Maps compression metadata rows from the database to decoded payloads.
 */
export const mapCompressionMetadataRows = (
    rows: CompressionMetadataQueryRow[]
): CompressionMetadataDecoded[] => {
    return rows.map(
        ({
            _id,
            clp_config: clpConfig,
            compressed_size: compressedSize,
            duration,
            start_time: startTime,
            status,
            status_msg: statusMsg,
            uncompressed_size: uncompressedSize,
            update_time: updateTime,
        }): CompressionMetadataDecoded => {
            const {clp_config} = decodeJobConfig(clpConfig);

            return {
                _id,
                compressed_size: compressedSize,
                clp_config,
                duration,
                start_time: startTime,
                status,
                status_msg: statusMsg,
                uncompressed_size: uncompressedSize,
                update_time: updateTime,
            };
        }
    );
};
