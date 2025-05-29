import type { SearchResult } from "./typings";
import settings from "../../../../../settings.json";

const SETTINGS_STORAGE_ENGINE = settings.ClpStorageEngine;
const STREAM_TYPE = SETTINGS_STORAGE_ENGINE === "clp" ? "ir" : "json";

/**
 * Returns the stream id based on the storage engine.
 * @param result
 * @param storageEngine
 */
function getStreamId(result: SearchResult): string {
    return SETTINGS_STORAGE_ENGINE === "clp" ? result.orig_file_id : result.archive_id;
}

export { STREAM_TYPE, getStreamId };
