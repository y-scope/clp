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


export {
    CLP_DEFAULT_DATASET_NAME,
    CLP_QUERY_ENGINES,
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
};
