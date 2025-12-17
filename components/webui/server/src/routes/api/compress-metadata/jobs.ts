import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {decode} from "@msgpack/msgpack";
import {
    CompressionMetadata,
    CompressionMetadataDecoded,
    CompressionMetadataDecodedSchema,
} from "@webui/common/schemas/compress-metadata";
import {ClpIoConfig} from "@webui/common/schemas/compression";
import {constants} from "http2";
import {RowDataPacket} from "mysql2";
import {brotliDecompressSync} from "node:zlib";

import settings from "../../../../settings.json" with {type: "json"};

type CompressionMetadataRow = CompressionMetadata;

/**
 * Decodes a compressed job config stored in the database.
 *
 * @param jobConfig
 * @return
 */
const decodeJobConfig = (
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
 * Compression metadata - jobs routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const mysqlConnectionPool = fastify.mysql.pool;

    fastify.get(
        "/",
        {
            schema: {
                querystring: Type.Object({
                    lastUpdateTimestampSeconds: Type.Optional(
                        Type.Number({minimum: 0})
                    ),
                }),
                response: {
                    [constants.HTTP_STATUS_OK]: Type.Array(
                        CompressionMetadataDecodedSchema
                    ),
                },
                tags: ["Compression Metadata"],
            },
        },
        async (request) => {
        const {lastUpdateTimestampSeconds} = request.query;
        const sinceSeconds = lastUpdateTimestampSeconds ??
            Math.floor(Date.now() / 1000);

            const [rows] = await mysqlConnectionPool.query<RowDataPacket[]>(
                `
                SELECT
                    UNIX_TIMESTAMP() as retrieval_time,
                    id as _id,
                    status,
                    status_msg,
                    start_time,
                    update_time,
                    duration,
                    uncompressed_size,
                    compressed_size,
                    clp_config
                FROM ${settings.SqlDbCompressionJobsTableName}
                WHERE update_time >= FROM_UNIXTIME(?) - 1
                ORDER BY _id DESC;
                `,
                [sinceSeconds]
            );

            return (rows as Array<RowDataPacket & CompressionMetadataRow>).map(
                ({
                    _id,
                    clp_config: clpConfig,
                    compressed_size: compressedSize,
                    duration,
                    retrieval_time: retrievalTime,
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
                        retrieval_time: retrievalTime,
                        start_time: startTime,
                        status,
                        status_msg: statusMsg,
                        uncompressed_size: uncompressedSize,
                        update_time: updateTime,
                    };
                }
            );
        }
    );
};

export default plugin;
