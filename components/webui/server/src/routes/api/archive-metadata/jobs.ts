import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {decode} from "@msgpack/msgpack";
import {constants} from "http2";
import {RowDataPacket} from "mysql2";
import {brotliDecompressSync} from "node:zlib";

import settings from "../../../../settings.json" with {type: "json"};
import {CompressionJobConfig} from "../../../plugins/app/CompressionJobDbManager/typings.js";


type CompressionJobRow = RowDataPacket & {
    _id: number;
    compressed_size: number;
    duration: number | null;
    retrieval_time: number;
    start_time: string | null;
    status: number;
    status_msg: string;
    uncompressed_size: number;
    update_time: string;
    clp_config?: Buffer | null;
};

type DecodedJobConfig = {
    dataset: string | null;
    paths: string[];
};

type CompressionJobResponseItem = {
    _id: number;
    compressed_size: number;
    dataset: string | null;
    duration: number | null;
    paths: string[];
    retrieval_time: number;
    start_time: string | null;
    status: number;
    status_msg: string;
    uncompressed_size: number;
    update_time: string;
};

/**
 * Decodes a compressed job config stored in the database.
 *
 * @param jobConfig
 * @return
 */
const decodeJobConfig = (jobConfig: unknown): DecodedJobConfig => {
    let dataset: string | null = null;
    let paths: string[] = [];

    if (jobConfig instanceof Buffer) {
        try {
            const decodedConfig = decode(
                brotliDecompressSync(jobConfig)
            ) as CompressionJobConfig;
            if ("string" === typeof decodedConfig?.input?.dataset) {
                dataset = decodedConfig.input.dataset;
            }
            if (Array.isArray(decodedConfig?.input?.paths_to_compress)) {
                const prefixToRemove = decodedConfig.input.path_prefix_to_remove;
                paths = decodedConfig.input.paths_to_compress.map((path) => {
                    if ("string" === typeof prefixToRemove &&
                        path.startsWith(prefixToRemove)
                    ) {
                        return path.substring(prefixToRemove.length);
                    }
                    return path;
                });
            }
        } catch (err: unknown) {
            // If decoding fails, fall back to empty dataset/paths but log for visibility.
            // eslint-disable-next-line no-console
            console.error(err);
        }
    }

    return {dataset, paths};
};

/**
 * Archive metadata - compression jobs routes.
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
                        Type.Object({
                            _id: Type.Number(),
                            compressed_size: Type.Number(),
                            dataset: Type.Union([Type.String(), Type.Null()]),
                            duration: Type.Union([Type.Number(), Type.Null()]),
                            paths: Type.Array(Type.String()),
                            retrieval_time: Type.Number(),
                            start_time: Type.Union([Type.String(), Type.Null()]),
                            status: Type.Number(),
                            status_msg: Type.String(),
                            uncompressed_size: Type.Number(),
                            update_time: Type.String(),
                        })
                    ),
                },
                tags: ["Archive Metadata"],
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

            return (rows as CompressionJobRow[]).map(
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
                }): CompressionJobResponseItem => {
                    const {dataset, paths} = decodeJobConfig(clpConfig);

                    return {
                        _id,
                        compressed_size: compressedSize,
                        dataset,
                        duration,
                        paths,
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
