import {settings} from "../settings";


const SETTINGS_LOG_INGESTOR_ENABLED = settings.LogIngestorEnabled;
const SETTINGS_LOGS_INPUT_TYPE = settings.LogsInputType;
const SETTINGS_MAX_DATASETS_PER_QUERY = settings.MaxDatasetsPerQuery;
const SETTINGS_PRESTO_MAX_NUM_SEARCH_RESULTS = settings.PrestoMaxNumSearchResults;
const SETTINGS_STORAGE_ENGINE = settings.ClpStorageEngine;
const SETTINGS_QUERY_ENGINE = settings.ClpQueryEngine;

export {
    SETTINGS_LOG_INGESTOR_ENABLED,
    SETTINGS_LOGS_INPUT_TYPE,
    SETTINGS_MAX_DATASETS_PER_QUERY,
    SETTINGS_PRESTO_MAX_NUM_SEARCH_RESULTS,
    SETTINGS_QUERY_ENGINE,
    SETTINGS_STORAGE_ENGINE,
};
