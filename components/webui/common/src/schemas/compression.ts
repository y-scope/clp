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
 * Schema for compression job metadata with decoded config.
 */
const CompressionJobWithConfigSchema = Type.Object({
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
});

type CompressionJobWithConfig = Static<typeof CompressionJobWithConfigSchema>;

export {
    AbsolutePathSchema,
    CompressionJobCreationSchema,
    CompressionJobSchema,
    CompressionJobWithConfigSchema,
    DATASET_NAME_MAX_LEN,
    DATASET_NAME_PATTERN,
    DatasetNameSchema,
};
export type {
    CompressionJob,
    CompressionJobCreation,
    CompressionJobWithConfig,
};
