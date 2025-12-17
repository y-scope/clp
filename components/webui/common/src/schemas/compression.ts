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
 * Compression job config (mirrors `ClpIoConfig` in
 * `components/job-orchestration/job_orchestration/scheduler/job_config.py`).
 */
enum CompressionJobInputType {
    FS = "fs",
    S3 = "s3",
}

type CompressionJobFsInputConfig = {
    type: CompressionJobInputType.FS;
    dataset?: string | null;
    paths_to_compress: string[];
    path_prefix_to_remove?: string | null;
    timestamp_key?: string | null;
    unstructured?: boolean;
};

type CompressionJobS3InputConfig = {
    type: CompressionJobInputType.S3;
    keys?: string[] | null;
    dataset?: string | null;
    timestamp_key?: string | null;
    unstructured?: boolean;
    // S3Config fields from Python `clp_py_utils.clp_config.S3Config` are not
    // enumerated here; extend as needed when S3 ingestion is wired up.
    [key: string]: unknown;
};

type CompressionJobOutputConfig = {
    tags?: string[] | null;
    target_archive_size: number;
    target_dictionaries_size: number;
    target_segment_size: number;
    target_encoded_file_size: number;
    compression_level: number;
};

const ClpIoFsInputConfigSchema = Type.Object({
    type: Type.Literal(CompressionJobInputType.FS),
    dataset: Type.Optional(Type.Union([Type.String(), Type.Null()])),
    paths_to_compress: Type.Array(Type.String()),
    path_prefix_to_remove: Type.Optional(Type.Union([Type.String(), Type.Null()])),
    timestamp_key: Type.Optional(Type.Union([Type.String(), Type.Null()])),
    unstructured: Type.Optional(Type.Boolean()),
});

const ClpIoS3InputConfigSchema = Type.Object(
    {
        type: Type.Literal(CompressionJobInputType.S3),
        keys: Type.Optional(Type.Array(Type.String())),
        dataset: Type.Optional(Type.Union([Type.String(), Type.Null()])),
        timestamp_key: Type.Optional(Type.Union([Type.String(), Type.Null()])),
        unstructured: Type.Optional(Type.Boolean()),
    },
    {additionalProperties: true}
);

const ClpIoOutputConfigSchema = Type.Object({
    tags: Type.Optional(Type.Union([Type.Array(Type.String()), Type.Null()])),
    target_archive_size: Type.Number(),
    target_dictionaries_size: Type.Number(),
    target_segment_size: Type.Number(),
    target_encoded_file_size: Type.Number(),
    compression_level: Type.Number(),
});

const ClpIoConfigSchema = Type.Object({
    input: Type.Union([ClpIoFsInputConfigSchema, ClpIoS3InputConfigSchema]),
    output: ClpIoOutputConfigSchema,
});

type ClpIoConfig = Static<typeof ClpIoConfigSchema>;

export {
    AbsolutePathSchema,
    CompressionJobCreationSchema,
    CompressionJobSchema,
    CompressionJobInputType,
    DATASET_NAME_MAX_LEN,
    DATASET_NAME_PATTERN,
    DatasetNameSchema,
    ClpIoConfigSchema,
    ClpIoFsInputConfigSchema,
    ClpIoOutputConfigSchema,
    ClpIoS3InputConfigSchema,
};
export type {
    CompressionJob,
    CompressionJobCreation,
    ClpIoConfig,
    CompressionJobFsInputConfig,
    CompressionJobS3InputConfig,
    CompressionJobOutputConfig,
};
