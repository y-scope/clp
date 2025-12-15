import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {Type} from "@sinclair/typebox";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {
    CompressionJobCreationSchema,
    CompressionJobSchema,
} from "@webui/common/schemas/compression";
import {ErrorSchema} from "@webui/common/schemas/error";
import {constants} from "http2";
import {RowDataPacket} from "mysql2";
import {brotliDecompressSync} from "node:zlib";
import {decode} from "@msgpack/msgpack";

import settings from "../../../../settings.json" with {type: "json"};
import {CompressionJobConfig} from "../../../plugins/app/CompressionJobDbManager/typings.js";
import {CONTAINER_INPUT_LOGS_ROOT_DIR} from "./typings.js";


/**
 * Default compression job configuration.
 */
const DEFAULT_COMPRESSION_JOB_CONFIG: CompressionJobConfig = Object.freeze({
    input: {
        paths_to_compress: [],
        path_prefix_to_remove: CONTAINER_INPUT_LOGS_ROOT_DIR,
    },
    output: {
        compression_level: settings.ArchiveOutputCompressionLevel,
        target_archive_size: settings.ArchiveOutputTargetArchiveSize,
        target_dictionaries_size: settings.ArchiveOutputTargetDictionariesSize,
        target_encoded_file_size: settings.ArchiveOutputTargetEncodedFileSize,
        target_segment_size: settings.ArchiveOutputTargetSegmentSize,
    },
});

/**
 * Compression API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {CompressionJobDbManager} = fastify;
    const mysqlConnectionPool = fastify.mysql.pool;

    const decodeJobConfig = (jobConfig: unknown) => {
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
                    const pathPrefixToRemove = decodedConfig.input.path_prefix_to_remove;
                    paths = decodedConfig.input.paths_to_compress.map((path) => {
                        if ("string" === typeof pathPrefixToRemove &&
                            path.startsWith(pathPrefixToRemove)
                        ) {
                            return path.substring(pathPrefixToRemove.length);
                        }
                        return path;
                    });
                }
            } catch (err: unknown) {
                fastify.log.error(err, "Failed to decode compression job config");
            }
        }

        return {dataset, paths};
    };

    /**
     * Submits a compression job and initiates the compression process.
     */
    fastify.post(
        "/",
        {
            schema: {
                body: CompressionJobCreationSchema,
                response: {
                    [constants.HTTP_STATUS_CREATED]: CompressionJobSchema,
                    [constants.HTTP_STATUS_INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Compression"],
            },
        },
        async (request, reply) => {
            const {
                paths,
                dataset,
                timestampKey,
            } = request.body;

            const jobConfig: CompressionJobConfig = structuredClone(DEFAULT_COMPRESSION_JOB_CONFIG);
            jobConfig.input.paths_to_compress = paths.map(
                (path) => CONTAINER_INPUT_LOGS_ROOT_DIR + path
            );

            if (CLP_STORAGE_ENGINES.CLP_S === settings.ClpStorageEngine as CLP_STORAGE_ENGINES) {
                if ("string" !== typeof dataset || 0 === dataset.length) {
                    request.log.error("Unable to submit compression job to the SQL database");
                } else {
                    jobConfig.input.dataset = dataset;
                }
                if ("undefined" !== typeof timestampKey) {
                    jobConfig.input.timestamp_key = timestampKey;
                }
            }

            try {
                const jobId = await CompressionJobDbManager.submitJob(jobConfig);
                reply.code(constants.HTTP_STATUS_CREATED);

                return {jobId};
            } catch (err: unknown) {
                const errMsg = "Unable to submit compression job to the SQL database";
                request.log.error(err, errMsg);

                return reply.internalServerError(errMsg);
            }
        }
    );

    /**
     * Retrieves compression jobs with decoded dataset and paths.
     */
    fastify.get(
        "/jobs",
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
                tags: ["Compression"],
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

            return (rows as Array<RowDataPacket & {
                _id: number;
                clp_config?: Buffer | null;
            }>).map((row) => {
                const {dataset, paths} = decodeJobConfig(row.clp_config);

                return {
                    ...row,
                    clp_config: undefined,
                    dataset,
                    paths,
                };
            });
        }
    );
};

export default plugin;
