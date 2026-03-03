/**
 * CLP query engines.
 */
enum CLP_QUERY_ENGINES {
    CLP = "clp",
    CLP_S = "clp-s",
    PRESTO = "presto",
}

/**
 * CLP variants.
 */
enum CLP_STORAGE_ENGINES {
    CLP = "clp",
    CLP_S = "clp-s",
}

/**
 * Matching the `StorageType` in `clp_py_utils.clp_config`.
 */
enum STORAGE_TYPE {
    FS = "fs",
    S3 = "s3",
}

/**
 * Matching the `CLP_DEFAULT_DATASET_NAME` in `clp_py_utils.clp_config`.
 */
const CLP_DEFAULT_DATASET_NAME = "default";

/**
 * Table suffixes for CLP database tables.
 * Matches constants in `clp_py_utils.clp_metadata_db_utils`.
 */
enum SqlTableSuffix {
    ARCHIVES = "archives",
    COLUMN_METADATA = "column_metadata",
    DATASETS = "datasets",
    FILES = "files",
}

/**
 * Matching the default `clp_table_prefix` in `clp_py_utils.clp_config`.
 */
const CLP_DEFAULT_TABLE_PREFIX = "clp_";


export {
    CLP_DEFAULT_DATASET_NAME,
    CLP_DEFAULT_TABLE_PREFIX,
    CLP_QUERY_ENGINES,
    CLP_STORAGE_ENGINES,
    SqlTableSuffix,
    STORAGE_TYPE,
};
