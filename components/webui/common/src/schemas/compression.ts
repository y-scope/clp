import {
    Static,
    Type,
} from "@sinclair/typebox";

import {
    CLP_DEFAULT_TABLE_PREFIX,
    SqlTableSuffix,
} from "../config.js";


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
 * Schema for request to create a new compression job.
 */
const CompressionJobCreationSchema = Type.Object({
    paths: Type.Array(AbsolutePathSchema, {minItems: 1}),
    dataset: Type.Optional(DatasetNameSchema),
    timestampKey: Type.Optional(Type.String()),
});

type CompressionJobCreation = Static<typeof CompressionJobCreationSchema>;

/**
 * Schema for compression job response.
 */
const CompressionJobSchema = Type.Object({
    jobId: Type.Number(),
});

type CompressionJob = Static<typeof CompressionJobSchema>;

/**
 * Matching the `InputType` class in `job_orchestration.scheduler.job_config`.
 */
enum CompressionJobInputType {
    FS = "fs",
    S3 = "s3",
}

/**
 * Matching `FsInputConfig` in `job_orchestration.scheduler.job_config`.
 */
const ClpIoFsInputConfigSchema = Type.Object({
    dataset: Type.Union([Type.String(),
        Type.Null()]),
    path_prefix_to_remove: Type.Union([Type.String(),
        Type.Null()]),
    paths_to_compress: Type.Array(Type.String()),
    timestamp_key: Type.Union([Type.String(),
        Type.Null()]),
    type: Type.Literal(CompressionJobInputType.FS),
    unstructured: Type.Boolean(),
});

/**
 * Matching `S3InputConfig` in `job_orchestration.scheduler.job_config`.
 */
const ClpIoS3InputConfigSchema = Type.Object({
    dataset: Type.Union([Type.String(),
        Type.Null()]),
    keys: Type.Union([Type.Array(Type.String()),
        Type.Null()]),
    timestamp_key: Type.Union([Type.String(),
        Type.Null()]),
    type: Type.Literal(CompressionJobInputType.S3),
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
        ]),
        output: ClpIoOutputConfigSchema,
    });

/**
 * Less strict version of `ClpIoConfigSchema` to prevent Fastify validation errors
 * for compression metadata from older CLP releases with missing data.
 */
const ClpIoPartialConfigSchema =
    Type.Object({
        input: Type.Union([Type.Partial(ClpIoFsInputConfigSchema),
            Type.Partial(ClpIoS3InputConfigSchema)]),
        output: Type.Partial(ClpIoOutputConfigSchema),
    });

type ClpIoConfig = Static<typeof ClpIoConfigSchema>;

type ClpIoS3InputConfig = Static<typeof ClpIoS3InputConfigSchema>;

type ClpIoFsInputConfig = Static<typeof ClpIoFsInputConfigSchema>;

export {
    ClpIoConfigSchema,
    ClpIoPartialConfigSchema,
    CompressionJobCreationSchema,
    CompressionJobInputType,
    CompressionJobSchema,
    DATASET_NAME_MAX_LEN,
    DATASET_NAME_PATTERN,
    DatasetNameSchema,
};
export type {
    ClpIoConfig,
    ClpIoFsInputConfig,
    ClpIoS3InputConfig,
    CompressionJob,
    CompressionJobCreation,
};
