import {
    Static,
    Type,
} from "@sinclair/typebox";

import {
    CLP_DEFAULT_TABLE_PREFIX,
    SqlTableSuffix,
} from "../config.js";
import {AwsAuthenticationSchema} from "./s3.js";


/**
 * Matching the `MYSQL_TABLE_NAME_MAX_LEN` in `clp_py_utils.clp_metadata_db_utils`.
 */
const MYSQL_TABLE_NAME_MAX_LEN = 64;

/**
 * Maximum length among all table suffixes.
 */
const TABLE_SUFFIX_MAX_LEN = Math.max(
    ...Object.values(SqlTableSuffix).map((suffix) => suffix.length)
);

/**
 * Dataset name validation constants matching `clp_package_utils.general.validate_dataset_name`.
 * - Pattern: only alphanumeric characters and underscores.
 * - Max length: computed using the default table prefix.
 */
const DATASET_NAME_PATTERN = "^\\w+$";
const DATASET_NAME_SEPARATOR_LEN = 1;
const DATASET_NAME_MAX_LEN =
    MYSQL_TABLE_NAME_MAX_LEN -
    CLP_DEFAULT_TABLE_PREFIX.length -
    DATASET_NAME_SEPARATOR_LEN -
    TABLE_SUFFIX_MAX_LEN;

/**
 * TypeBox schema for dataset name validation.
 */
const DatasetNameSchema = Type.String({
    pattern: DATASET_NAME_PATTERN,
    maxLength: DATASET_NAME_MAX_LEN,
});

/**
 * Schema for an absolute file system path.
 * - Must not be empty.
 * - Must start with "/": the path must be an absolute path.
 */
const AbsolutePathSchema = Type.String({
    minLength: 1,
    pattern: "^/",
});

/**
 * Matching the `InputType` class in `job_orchestration.scheduler.job_config`.
 */
enum CompressionJobInputType {
    FS = "fs",
    S3 = "s3",
    S3_OBJECT_METADATA = "s3_object_metadata",
}

/**
 * Common CLP-S fields shared by all compression job creation schemas.
 */
const clpSJobCreationFields = {
    dataset: Type.Optional(DatasetNameSchema),
    timestampKey: Type.Optional(Type.String()),
    unstructured: Type.Optional(Type.Boolean()),
};

/**
 * Schema for request to create a new FS compression job.
 */
const FsCompressionJobCreationSchema = Type.Object({
    ...clpSJobCreationFields,
    inputType: Type.Literal(CompressionJobInputType.FS),
    paths: Type.Array(AbsolutePathSchema, {minItems: 1}),
});

type FsCompressionJobCreation = Static<typeof FsCompressionJobCreationSchema>;

/**
 * Schema for request to create a new S3 compression job.
 *
 * When `scanner` is true, the request is forwarded to the log-ingestor as an
 * S3 scanner ingestion job instead of a one-time compression job.
 */
const S3CompressionJobCreationSchema = Type.Object({
    ...clpSJobCreationFields,
    bucket: Type.String({minLength: 1}),
    bufferChannelCapacity: Type.Optional(Type.Number({minimum: 1})),
    bufferFlushThresholdBytes: Type.Optional(Type.Number({minimum: 1})),
    bufferTimeoutSec: Type.Optional(Type.Number({minimum: 1})),
    inputType: Type.Literal(CompressionJobInputType.S3),
    keyPrefix: Type.Optional(Type.String()),
    keys: Type.Optional(Type.Array(Type.String({minLength: 1}), {minItems: 1})),
    regionCode: Type.String({minLength: 1}),
    scanner: Type.Boolean(),
    scanningIntervalSec: Type.Optional(Type.Number({minimum: 1})),
});

type S3CompressionJobCreation = Static<typeof S3CompressionJobCreationSchema>;

/**
 * Schema for request to create a new compression job (FS or S3).
 */
const CompressionJobCreationSchema = Type.Union([
    FsCompressionJobCreationSchema,
    S3CompressionJobCreationSchema,
]);

type CompressionJobCreation = Static<typeof CompressionJobCreationSchema>;

/**
 * Schema for compression job response.
 */
const CompressionJobSchema = Type.Object({
    jobId: Type.Number(),
});

type CompressionJob = Static<typeof CompressionJobSchema>;

/**
 * Schema for S3 scanner job creation response.
 */
const ScannerJobResponseSchema = Type.Object({
    jobIds: Type.Array(Type.Number()),
});

type ScannerJobResponse = Static<typeof ScannerJobResponseSchema>;


const clpSInputConfigFields = {
    dataset: Type.Union([Type.String(),
        Type.Null()]),
    timestamp_key: Type.Union([Type.String(),
        Type.Null()]),
    unstructured: Type.Boolean(),
};

/**
 * Matching `FsInputConfig` in `job_orchestration.scheduler.job_config`.
 */
const ClpIoFsInputConfigSchema = Type.Object({
    ...clpSInputConfigFields,
    path_prefix_to_remove: Type.Union([Type.String(),
        Type.Null()]),
    paths_to_compress: Type.Array(Type.String()),
    type: Type.Literal(CompressionJobInputType.FS),
});

type ClpIoFsInputConfig = Static<typeof ClpIoFsInputConfigSchema>;

/**
 * Matching `S3InputConfig` in `job_orchestration.scheduler.job_config`.
 * Includes fields inherited from `S3Config` in `clp_py_utils.clp_config`.
 */
const ClpIoS3InputConfigSchema = Type.Object({
    ...clpSInputConfigFields,
    aws_authentication: AwsAuthenticationSchema,
    bucket: Type.String(),
    key_prefix: Type.String(),
    keys: Type.Union([Type.Array(Type.String()),
        Type.Null()]),
    region_code: Type.Union([Type.String(),
        Type.Null()]),
    type: Type.Literal(CompressionJobInputType.S3),
});

type ClpIoS3InputConfig = Static<typeof ClpIoS3InputConfigSchema>;

/**
 * Matching `S3ObjectMetadataInputConfig` in `job_orchestration.scheduler.job_config`.
 */
const ClpIoS3ObjectMetadataInputConfigSchema = Type.Object({
    dataset: Type.Union([Type.String(),
        Type.Null()]),
    timestamp_key: Type.Union([Type.String(),
        Type.Null()]),
    type: Type.Literal(CompressionJobInputType.S3_OBJECT_METADATA),
    unstructured: Type.Boolean(),
});

/**
 * Matching `OutputConfig` in `job_orchestration.scheduler.job_config`.
 */
const ClpIoOutputConfigSchema = Type.Object({
    compression_level: Type.Number(),
    target_archive_size: Type.Number(),
    target_dictionaries_size: Type.Number(),
    target_encoded_file_size: Type.Number(),
    target_segment_size: Type.Number(),
});

/**
 * Matching `ClpIoConfig` in `job_orchestration.scheduler.job_config`.
 */
const ClpIoConfigSchema =
    Type.Object({
        input: Type.Union([
            ClpIoFsInputConfigSchema,
            ClpIoS3InputConfigSchema,
            ClpIoS3ObjectMetadataInputConfigSchema,
        ]),
        output: ClpIoOutputConfigSchema,
    });

type ClpIoConfig = Static<typeof ClpIoConfigSchema>;

/**
 * Less strict version of `ClpIoConfigSchema` to prevent Fastify validation errors
 * for compression metadata from older CLP releases with missing data.
 */
const ClpIoPartialConfigSchema =
    Type.Object({
        input: Type.Union([Type.Partial(ClpIoFsInputConfigSchema),
            Type.Partial(ClpIoS3InputConfigSchema),
            Type.Partial(ClpIoS3ObjectMetadataInputConfigSchema)]),
        output: Type.Partial(ClpIoOutputConfigSchema),
    });

export {
    ClpIoConfigSchema,
    ClpIoPartialConfigSchema,
    CompressionJobCreationSchema,
    CompressionJobInputType,
    CompressionJobSchema,
    DATASET_NAME_MAX_LEN,
    DATASET_NAME_PATTERN,
    DatasetNameSchema,
    ScannerJobResponseSchema,
};
export type {
    ClpIoConfig,
    ClpIoFsInputConfig,
    ClpIoS3InputConfig,
    CompressionJob,
    CompressionJobCreation,
    FsCompressionJobCreation,
    S3CompressionJobCreation,
    ScannerJobResponse,
};
