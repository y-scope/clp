import {join} from "node:path";

import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {
    ClpIoConfig,
    ClpIoFsInputConfig,
    CompressionJobCreationSchema,
    CompressionJobInputType,
    CompressionJobSchema,
} from "@webui/common/schemas/compression";
import {ErrorSchema} from "@webui/common/schemas/error";
import {constants} from "http2";

import settings from "../../../../settings.json" with {type: "json"};
import {CONTAINER_INPUT_LOGS_ROOT_DIR} from "./typings.js";


/**
 * Default compression job configuration.
 */
const DEFAULT_COMPRESSION_JOB_CONFIG: ClpIoConfig = Object.freeze({
    input: {
        dataset: null,
        path_prefix_to_remove: CONTAINER_INPUT_LOGS_ROOT_DIR,
        paths_to_compress: [],
        timestamp_key: null,
        type: CompressionJobInputType.FS,
        unstructured: true,
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
                unstructured,
            } = request.body;

            const jobConfig: ClpIoConfig = structuredClone(DEFAULT_COMPRESSION_JOB_CONFIG);
            // eslint-disable-next-line no-warning-comments
            // TODO: Add support for S3 input
            (jobConfig.input as ClpIoFsInputConfig).paths_to_compress = paths.map(
                (path) => join(settings.LogsInputRootDir, path)
            );

            if (CLP_STORAGE_ENGINES.CLP_S === settings.ClpStorageEngine as CLP_STORAGE_ENGINES) {
                jobConfig.input.unstructured = false;
                if ("string" !== typeof dataset || 0 === dataset.length) {
                    request.log.error("Unable to submit compression job to the SQL database");
                } else {
                    jobConfig.input.dataset = dataset;
                }
                if ("undefined" !== typeof timestampKey) {
                    jobConfig.input.timestamp_key = timestampKey;
                }
                if (true === unstructured) {
                    jobConfig.input.unstructured = true;
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
};

export default plugin;
