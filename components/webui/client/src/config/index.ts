import {settings} from "../settings";


/**
 * CLP variants.
 */
enum CLP_STORAGE_ENGINES {
    CLP = "clp",
    CLP_S = "clp-s",
}

/**
 * Query engine options.
 */
enum CLP_QUERY_ENGINES {
    NATIVE = "native",
    PRESTO = "presto",
}

const SETTINGS_STORAGE_ENGINE = settings.ClpStorageEngine as CLP_STORAGE_ENGINES;
const SETTINGS_QUERY_ENGINE = settings.ClpQueryEngine as CLP_QUERY_ENGINES;

/**
 * Stream type based on the storage engine.
 */
const STREAM_TYPE = CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE ?
    "ir" :
    "json";

export {
    CLP_STORAGE_ENGINES,
    CLP_QUERY_ENGINES,
    SETTINGS_STORAGE_ENGINE,
    SETTINGS_QUERY_ENGINE,
    STREAM_TYPE,
};
