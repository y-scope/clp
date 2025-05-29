import {TableProps} from "antd";
import dayjs from "dayjs";

import {DATETIME_FORMAT_TEMPLATE} from "../../../../typings/datetime";
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
    archive_id?: string;
    log_event_ix: number;
}

// Helper: is IR stream
const IS_IR_STREAM = process.env.REACT_APP_CLP_STORAGE_ENGINE === "clp";
const STREAM_TYPE = IS_IR_STREAM ? "ir" : "json";

// Helper: get streamId
const getStreamId = (result: SearchResult) => (
    IS_IR_STREAM ? result.orig_file_id : result.archive_id
);

/**
 * Columns configuration for the seach results table.
 */
const searchResultsTableColumns: NonNullable<TableProps<SearchResult>["columns"]> = [
    {
        dataIndex: "timestamp",
        key: "timestamp",
        render: (timestamp: number) => dayjs(timestamp).format(DATETIME_FORMAT_TEMPLATE),
        sorter: true,
        title: "Timestamp",
        width: 15,
    },
    {
        dataIndex: "message",
        key: "message",
        render: (_, record) => (
            <Message
                filePath={record.orig_file_path}
                message={record.message}
                streamId={getStreamId(record) || ""}
                streamType={STREAM_TYPE}
                logEventIdx={record.log_event_ix}
            />
        ),
        title: "Message",
        width: 85,
    },
];

export type {SearchResult};
export {searchResultsTableColumns};
