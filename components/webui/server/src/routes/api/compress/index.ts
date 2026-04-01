import {join} from "node:path";

import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {
    AwsAuthentication,
    ClpIoConfig,
    ClpIoFsInputConfig,
    ClpIoS3InputConfig,
    CompressionJobCreationSchema,
    CompressionJobInputType,
    CompressionJobSchema,
    FsCompressionJobCreation,
    S3CompressionJobCreation,
} from "@webui/common/schemas/compression";
import {ErrorSchema} from "@webui/common/schemas/error";
import {constants} from "http2";

import settings from "../../../../settings.json" with {type: "json"};
import {CONTAINER_INPUT_LOGS_ROOT_DIR} from "./typings.js";


/**
 * Default compression output configuration.
 */
const DEFAULT_OUTPUT_CONFIG = Object.freeze({
    compression_level: settings.ArchiveOutputCompressionLevel,
    target_archive_size: settings.ArchiveOutputTargetArchiveSize,
    target_dictionaries_size: settings.ArchiveOutputTargetDictionariesSize,
    target_encoded_file_size: settings.ArchiveOutputTargetEncodedFileSize,
    target_segment_size: settings.ArchiveOutputTargetSegmentSize,
});

/**
 * Applies CLP-S specific fields (dataset, timestamp_key, unstructured) to the job config input.
 *
 * @param input The input config to modify.
 * @param body The request body containing CLP-S fields.
 * @param body.dataset
 * @param body.timestampKey
 * @param body.unstructured
 * @param log Fastify logger for error reporting.
 * @param log.error
 */
const applyClpSFields = (
    input: ClpIoFsInputConfig | ClpIoS3InputConfig,
    body: {dataset?: string; timestampKey?: string; unstructured?: boolean},
    log: {error: (msg: string) => void},
) => {
    input.unstructured = false;
    if ("string" !== typeof body.dataset || 0 === body.dataset.length) {
        log.error("Unable to submit compression job to the SQL database");
    } else {
        input.dataset = body.dataset;
    }
    if ("undefined" !== typeof body.timestampKey) {
        input.timestamp_key = body.timestampKey;
    }
    if (true === body.unstructured) {
        input.unstructured = true;
    }
};

/**
 * Builds a ClpIoConfig for a filesystem compression job.
 *
 * @param body The request body.
 * @param log Fastify logger.
 * @param log.error
 * @return The job configuration.
 */
const buildFsJobConfig = (
    body: FsCompressionJobCreation,
    log: {error: (msg: string) => void},
): ClpIoConfig => {
    const input: ClpIoFsInputConfig = {
        dataset: null,
        path_prefix_to_remove: CONTAINER_INPUT_LOGS_ROOT_DIR,
        paths_to_compress: body.paths.map(
            (path) => join(settings.LogsInputRootDir, path)
        ),
        timestamp_key: null,
        type: CompressionJobInputType.FS,
        unstructured: true,
    };

    if (CLP_STORAGE_ENGINES.CLP_S === settings.ClpStorageEngine as CLP_STORAGE_ENGINES) {
        applyClpSFields(input, body, log);
    }

    const output = structuredClone(DEFAULT_OUTPUT_CONFIG);

    return {input, output};
};

/**
 * Builds a ClpIoConfig for an S3 compression job.
 *
 * @param body The request body.
 * @param log Fastify logger.
 * @param log.error
 * @return The job configuration.
 * @throws Error if S3 AWS authentication is not configured.
 */
const buildS3JobConfig = (
    body: S3CompressionJobCreation,
    log: {error: (msg: string) => void},
): ClpIoConfig => {
    const awsAuth = settings.LogsInputS3AwsAuthentication as AwsAuthentication | null;
    if (null === awsAuth) {
        throw new Error("S3 AWS authentication is not configured in server settings.");
    }

    const input: ClpIoS3InputConfig = {
        aws_authentication: awsAuth,
        bucket: body.bucket,
        dataset: null,
        endpoint_url: body.endpointUrl ?? null,
        key_prefix: body.keyPrefix ?? "",
        keys: body.keys ?? null,
        region_code: body.regionCode ?? null,
        timestamp_key: null,
        type: CompressionJobInputType.S3,
        unstructured: true,
    };

    if (CLP_STORAGE_ENGINES.CLP_S === settings.ClpStorageEngine as CLP_STORAGE_ENGINES) {
        applyClpSFields(input, body, log);
    }

    const output = structuredClone(DEFAULT_OUTPUT_CONFIG);

    return {input, output};
};

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
            try {
                let jobConfig: ClpIoConfig;
                if (CompressionJobInputType.S3 === request.body.inputType) {
                    jobConfig = buildS3JobConfig(
                        request.body as S3CompressionJobCreation,
                        request.log,
                    );
                } else {
                    jobConfig = buildFsJobConfig(
                        request.body as FsCompressionJobCreation,
                        request.log,
                    );
                }

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
