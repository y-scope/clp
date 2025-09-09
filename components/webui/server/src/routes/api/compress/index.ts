import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

import settings from "../../../../settings.json" with {type: "json"};
import {CompressionJobConfig} from "../../../plugins/app/CompressionJobDbManager/index.js";
import {
    CompressionJobCreationSchema,
    CompressionJobSchema,
} from "../../../schemas/compression.js";
import {ErrorSchema} from "../../../schemas/error.js";


const DEFAULT_PATH_PREFIX = "/mnt/logs";

/**
 * Default compression job configuration.
 */
const DEFAULT_COMPRESSION_JOB_CONFIG: CompressionJobConfig = Object.freeze({
    input: {
        paths_to_compress: [],
        path_prefix_to_remove: DEFAULT_PATH_PREFIX,
    },
    output: {
        compression_level: 3,
        target_archive_size: 268435456,
        target_dictionaries_size: 33554432,
        target_encoded_file_size: 268435456,
        target_segment_size: 268435456,
    },
});

/**
 * Compression API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {CompressionJobDbManager} = fastify;

    /**
     * Submits a compression job and initiates the compression process.
     */
    fastify.post(
        "/",
        {
            schema: {
                body: CompressionJobCreationSchema,
                response: {
                    [StatusCodes.CREATED]: CompressionJobSchema,
                    [StatusCodes.INTERNAL_SERVER_ERROR]: ErrorSchema,
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
            jobConfig.input.paths_to_compress = paths.map((path) => DEFAULT_PATH_PREFIX + path);

            if ("clp-s" === settings.ClpStorageEngine) {
                if ("undefined" !== typeof dataset) {
                    jobConfig.input.dataset = dataset;
                }
                if ("undefined" !== typeof timestampKey) {
                    jobConfig.input.timestamp_key = timestampKey;
                }
            }

            try {
                const jobId = await CompressionJobDbManager.submitJob(jobConfig);
                reply.code(StatusCodes.CREATED);

                return {jobId};
            } catch (err: unknown) {
                const errMsg = "Unable to submit compression job to the SQL database";
                request.log.error(err, errMsg);

                return reply.internalServerError(errMsg);
            }
        }
    );
};

export default plugin;
