import {TableProps} from "antd";
import dayjs from "dayjs";

import Message from "./Message";


/**
 * Structure of search results data displayed in the table.
 */
interface SearchResult {
    _id: string;
    timestamp: number;
    message: string;
    filePath: string;
    orig_file_path: string;
    orig_file_id: string;
    log_event_ix: number;
}
const DATETIME_FORMAT_TEMPLATE = "YYYY-MMM-DD HH:mm:ss";
/**
 * Columns configuration for the seach results table.
 */
const searchResultsTableColumns: NonNullable<TableProps<SearchResult>["columns"]> = [
    {
        dataIndex: "timestamp",
        key: "timestamp",
        sorter: true,
        title: "Timestamp",
        width: 15,
        ellipsis: {
            showTitle: false,
        },
        render: (timestamp: number) => dayjs(timestamp).format(DATETIME_FORMAT_TEMPLATE),
    },
    {
        dataIndex: "message",
        key: "message",
        render: (_, record) => (
            <Message
                filePath={record.orig_file_path}
                message={record.message}/>
        ),
        title: "Message",
        width: 85,
    },
];

export type {SearchResult};
export {searchResultsTableColumns};
