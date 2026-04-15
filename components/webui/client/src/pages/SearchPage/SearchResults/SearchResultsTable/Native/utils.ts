import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import dayjs from "dayjs";

import {SETTINGS_STORAGE_ENGINE} from "../../../../../config";
import type {SearchResult} from "./SearchResultsVirtualTable/typings";


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

/**
 * Formats a numeric timestamp of an event as an ISO 8601 string.
 *
 * @param timestamp
 * @return ISO 8601 string.
 */
const getExportEventTimestamp = (timestamp: number): string => (
    dayjs(timestamp).toISOString()
);

/**
 * Returns a filesystem-safe timestamp string suitable for export filenames.
 *
 * @return ISO-8601-like string with colons and dots replaced by dashes.
 */
const getExportFilenameTimestamp = (): string => (
    dayjs().format("YYYY-MM-DDTHH-mm-ss-SSS[Z]")
);


export {
    getExportEventTimestamp,
    getExportFilenameTimestamp,
    getStreamId,
};
