import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import dayjs from "dayjs";

import {SETTINGS_STORAGE_ENGINE} from "../../../../../config";
import {DATETIME_FORMAT_TEMPLATE} from "../../../../../typings/datetime";
import type {SearchResult} from "./SearchResultsVirtualTable/typings";


/**
 * Formats a numeric timestamp of an event as an ISO 8601 string.
 *
 * @param timestamp
 * @return ISO 8601 string.
 */
const formatExportEventTimestamp = (timestamp: number): string => (
    dayjs.utc(timestamp).toISOString()
);

/**
 * Returns a filesystem-safe timestamp string suitable for export filenames.
 *
 * @return ISO-8601-like string with colons and dots replaced by dashes.
 */
const formatExportFilenameTimestamp = (): string => (
    dayjs().format("YYYY-MM-DDTHH-mm-ss-SSS[Z]")
);

/**
 * Serializes a search result as a JSONL line. If the message field is valid
 * JSON it is included as a parsed object; otherwise it is kept as a string.
 *
 * @param result
 * @return A single JSON line (without trailing newline).
 */
const formatResultAsJsonl = (result: SearchResult): string => {
    let messageValue: unknown;
    try {
        messageValue = JSON.parse(result.message);
    } catch {
        messageValue = result.message;
    }

    return JSON.stringify({
        timestamp: formatExportEventTimestamp(result.timestamp),
        message: messageValue,
    });
};

/**
 * Formats a numeric timestamp as a UTC datetime string.
 *
 * @param timestamp
 * @return The formatted datetime string.
 */
const formatTimestamp = (timestamp: number): string => (
    dayjs.utc(timestamp).format(DATETIME_FORMAT_TEMPLATE)
);

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

export {
    formatExportEventTimestamp,
    formatExportFilenameTimestamp,
    formatResultAsJsonl,
    formatTimestamp,
    getStreamId,
};
