import {brotliDecompressSync} from "node:zlib";

import {decode} from "@msgpack/msgpack";
import {Value} from "@sinclair/typebox/value";
import {CompressionMetadataDecoded} from "@webui/common/schemas/compress-metadata";
import {
    ClpIoConfig,
    ClpIoConfigSchema,
} from "@webui/common/schemas/compression";

import {CompressionMetadataQueryRow} from "./sql.js";


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
    if (!(jobConfig instanceof Buffer)) {
        throw new Error("Missing clp_config buffer for compression metadata");
    }

    try {
        const decodedClpConfig = Value.Parse(
            ClpIoConfigSchema,
            decode(brotliDecompressSync(jobConfig))
        );

        return {clp_config: decodedClpConfig};
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
            start_time: startTime,
            status,
            status_msg: statusMsg,
            uncompressed_size: uncompressedSize,
            update_time: updateTime,
        }): CompressionMetadataDecoded => {
            const {clp_config: decodedClpConfig} = decodeJobConfig(clpConfig);

            return {
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
        }
    );
};

export {
    decodeJobConfig,
    mapCompressionMetadataRows,
};
