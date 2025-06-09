import {TableProps} from "antd";
import dayjs from "dayjs";

import {DATETIME_FORMAT_TEMPLATE} from "../../../../typings/datetime";
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
        defaultSortOrder: "ascend",
        key: "timestamp",
        render: (timestamp: number) => dayjs(timestamp).format(DATETIME_FORMAT_TEMPLATE),
        sorter: (a, b) => a.timestamp - b.timestamp,

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
                filePath={record.orig_file_path}
                logEventIdx={record.log_event_ix}
                message={record.message}
                streamId={getStreamId(record)}/>
        ),
        title: "Message",
        width: 85,
    },
];

/**
 * Padding for the table to the bottom of the page.
 */
const TABLE_BOTTOM_PADDING = 75;


export type {SearchResult};
export {
    searchResultsTableColumns, TABLE_BOTTOM_PADDING,
};
