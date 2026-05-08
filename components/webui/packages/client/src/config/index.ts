import {
    CLP_QUERY_ENGINES,
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";

import {settings} from "../settings";


const SETTINGS_LOGS_INPUT_TYPE = settings.LogsInputType as STORAGE_TYPE;
const SETTINGS_MAX_DATASETS_PER_QUERY = settings.MaxDatasetsPerQuery;
const SETTINGS_STORAGE_ENGINE = settings.ClpStorageEngine as CLP_STORAGE_ENGINES;
const SETTINGS_QUERY_ENGINE = settings.ClpQueryEngine as CLP_QUERY_ENGINES;

/**
 * Stream type based on the storage engine.
 */
const STREAM_TYPE = CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE ?
    "ir" :
    "json";

export {
    SETTINGS_LOGS_INPUT_TYPE,
    SETTINGS_MAX_DATASETS_PER_QUERY,
    SETTINGS_QUERY_ENGINE,
    SETTINGS_STORAGE_ENGINE,
    STREAM_TYPE,
};
