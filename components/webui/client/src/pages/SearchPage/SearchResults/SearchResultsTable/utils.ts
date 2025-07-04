import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../../config";
import type {SearchResult} from "./typings";


/**
 * Returns the stream id based on the storage engine.
 *
 * @param result The search result object.
 * @return The stream id string, either from orig_file_id or archive_id.
 */
const getStreamId = (result: SearchResult): string => {
    return CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE ?
        result.orig_file_id :
        result.archive_id;
};


export {getStreamId};
