import {join} from "node:path";

import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {
    ClpIoConfig,
    ClpIoFsInputConfig,
    ClpIoS3InputConfig,
    CompressionJobCreation,
    CompressionJobCreationSchema,
    CompressionJobInputType,
    CompressionJobSchema,
    FsCompressionJobCreation,
    S3CompressionJobCreation,
    ScannerJobResponseSchema,
} from "@webui/common/schemas/compression";
import {ErrorSchema} from "@webui/common/schemas/error";
import {AwsAuthType} from "@webui/common/schemas/s3";
import {Nullable} from "@webui/common/utility-types";
import {FastifyBaseLogger} from "fastify";
import {constants} from "http2";

import settings from "../../../../settings.json" with {type: "json"};
import {handleScannerSubmission} from "./scanner.js";
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
 * Applies CLP-S fields to a compression job payload.
 *
 * @param body
 * @param log
 * @param input
 */
const applyClpSFields = (
    body: CompressionJobCreation,
    log: FastifyBaseLogger,
    input: ClpIoFsInputConfig | ClpIoS3InputConfig,
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
 * @param body
 * @param log
 * @return The job configuration.
 */
const buildFsJobConfig = (
    body: FsCompressionJobCreation,
    log: FastifyBaseLogger
): ClpIoConfig => {
    const input: ClpIoFsInputConfig = {
        type: CompressionJobInputType.FS,

        path_prefix_to_remove: CONTAINER_INPUT_LOGS_ROOT_DIR,
        paths_to_compress: body.paths.map(
            (path) => join(settings.LogsInputRootDir, path)
        ),

        dataset: null,
        timestamp_key: null,
        unstructured: true,
    };

    if (CLP_STORAGE_ENGINES.CLP_S === settings.ClpStorageEngine as CLP_STORAGE_ENGINES) {
        applyClpSFields(body, log, input);
    }

    return {
        input: input,
        output: structuredClone(DEFAULT_OUTPUT_CONFIG),
    };
};

/**
 * Builds a ClpIoConfig for an S3 compression job.
 *
 * @param body
 * @param log
 * @param awsAccessKeyId AWS access key ID from env config.
 * @param awsSecretAccessKey AWS secret access key from env config.
 * @return The job configuration.
 * @throws Error if S3 AWS authentication is not configured.
 */
const buildS3JobConfig = (
    body: S3CompressionJobCreation,
    log: FastifyBaseLogger,
    awsAccessKeyId: string,
    awsSecretAccessKey: string,
): ClpIoConfig => {
    const authType = settings.LogsInputS3AwsAuthType as Nullable<AwsAuthType>;
    if (null === authType) {
        throw new Error("S3 AWS authentication is not configured in server settings.");
    }

    const input: ClpIoS3InputConfig = {
        type: CompressionJobInputType.S3,

        aws_authentication: {
            type: authType,
            profile: settings.LogsInputS3AwsProfile,
            credentials: {
                access_key_id: awsAccessKeyId,
                secret_access_key: awsSecretAccessKey,
                session_token: null,
            },
        },
        bucket: body.bucket,
        key_prefix: body.keyPrefix ?? "",
        keys: body.keys ?? null,
        region_code: body.regionCode,

        dataset: null,
        timestamp_key: null,
        unstructured: false,
    };

    if (CLP_STORAGE_ENGINES.CLP_S === settings.ClpStorageEngine as CLP_STORAGE_ENGINES) {
        applyClpSFields(body, log, input);
    }

    return {
        input: input,
        output: structuredClone(DEFAULT_OUTPUT_CONFIG),
    };
};

/**
 * Compression API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {CompressionJobDbManager} = fastify;

    /**
     * Submits a compression job or scanner ingestion job.
     */
    fastify.post(
        "/",
        {
            schema: {
                body: CompressionJobCreationSchema,
                response: {
                    [constants.HTTP_STATUS_OK]: ScannerJobResponseSchema,
                    [constants.HTTP_STATUS_CREATED]: CompressionJobSchema,
                    [constants.HTTP_STATUS_INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Compression"],
            },
        },
        async (request, reply) => {
            try {
                if (
                    CompressionJobInputType.S3 === request.body.inputType &&
                    request.body.scanner
                ) {
                    return await handleScannerSubmission(
                        request.body,
                        request.log,
                    );
                }

                const jobConfig: ClpIoConfig =
                    (CompressionJobInputType.S3 === request.body.inputType) ?
                        buildS3JobConfig(
                            request.body,
                            request.log,
                            fastify.config.CLP_LOGS_INPUT_AWS_ACCESS_KEY_ID,
                            fastify.config.CLP_LOGS_INPUT_AWS_SECRET_ACCESS_KEY,
                        ) :
                        buildFsJobConfig(request.body, request.log);

                const jobId = await CompressionJobDbManager.submitJob(jobConfig);
                reply.code(constants.HTTP_STATUS_CREATED);

                return {jobId};
            } catch (err: unknown) {
                const errMsg = err instanceof Error ?
                    err.message :
                    "Unable to submit job";

                request.log.error(err, errMsg);

                return reply.internalServerError(errMsg);
            }
        }
    );
};

export default plugin;
