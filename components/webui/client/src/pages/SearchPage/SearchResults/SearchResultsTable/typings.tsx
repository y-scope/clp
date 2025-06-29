import {TableProps} from "antd";
import dayjs from "dayjs";

import {DATETIME_FORMAT_TEMPLATE} from "../../../../typings/datetime";
import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from ".././../../../config";
import Message from "./Message";
import {getStreamId} from "./utils";


/**
 * Structure of search results data displayed in the table.
 */
interface SearchResult {
    _id: string;
    archive_id: string;
    filePath: string;
    log_event_ix: number;
    message: string;
    orig_file_id: string;
    orig_file_path: string;
    timestamp: number;
}

/**
 * Columns configuration for the seach results table.
 */
const searchResultsTableColumns: NonNullable<TableProps<SearchResult>["columns"]> = [
    {
        dataIndex: "timestamp",
        defaultSortOrder: "descend",
        key: "timestamp",
        render: (timestamp: number) => dayjs.utc(timestamp).format(DATETIME_FORMAT_TEMPLATE),
        sorter: (a, b) => {
            const timestampDiff = a.timestamp - b.timestamp;
            if (0 !== timestampDiff) {
                return timestampDiff;
            }

            return a._id.localeCompare(b._id);
        },

        // Specifying a third sort direction removes ability for user to cancel sorting.
        sortDirections: [
            "ascend",
            "descend",
            "ascend",
        ],
        title: "Timestamp",
        width: 15,
    },
    {
        dataIndex: "message",
        key: "message",
        render: (_, record) => (
            <Message
                logEventIdx={record.log_event_ix}
                message={record.message}
                streamId={getStreamId(record)}
                fileText={
                    CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE ?
                        record.orig_file_path :
                        " Original file"
                }/>
        ),
        title: "Message",
        width: 85,
    },
];

/**
 * Padding for the table to the bottom of the page.
 */
const TABLE_BOTTOM_PADDING = 75;

/**
 * The maximum number of results to retrieve for a search.
 */
const SEARCH_MAX_NUM_RESULTS = 1000;


export type {SearchResult};
export {
    SEARCH_MAX_NUM_RESULTS,
    searchResultsTableColumns,
    TABLE_BOTTOM_PADDING,
};
