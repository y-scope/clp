import settings from "../../settings.json";

const SETTINGS_STORAGE_ENGINE = settings.ClpStorageEngine;

/**
 * CLP variants.
 */
enum CLP_STORAGE_ENGINES {
    CLP = "clp",
    CLP_S = "clp-s",
}

/**
 * Stream type based on the storage engine.
 */
const STREAM_TYPE = CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE ?
    "ir" :
    "json";

export {
    SETTINGS_STORAGE_ENGINE,
    STREAM_TYPE,
    CLP_STORAGE_ENGINES,
};
