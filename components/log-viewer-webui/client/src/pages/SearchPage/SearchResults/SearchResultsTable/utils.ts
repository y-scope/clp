import settings from "../../../../../settings.json";
import type {SearchResult} from "./typings";


const SETTINGS_STORAGE_ENGINE = settings.ClpStorageEngine;

/**
 * Stream type based on the storage engine (i.e. clp vs. clp-s).
 */
const STREAM_TYPE = "clp" === SETTINGS_STORAGE_ENGINE ?
    "ir" :
    "json";

/**
 * Returns the stream id based on the storage engine.
 *
 * @param result The search result object.
 * @return The stream id string, either from orig_file_id or archive_id.
 */
const getStreamId = (result: SearchResult): string => {
    return "clp" === SETTINGS_STORAGE_ENGINE ?
        result.orig_file_id :
        result.archive_id;
};


export {
    getStreamId,
    STREAM_TYPE,
};
