import {CopyOutlined} from "@ant-design/icons";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {
    Button,
    TableProps,
    Tooltip,
} from "antd";
import dayjs from "dayjs";

import {SETTINGS_STORAGE_ENGINE} from "../../../../../../config";
import {DATETIME_FORMAT_TEMPLATE} from "../../../../../../typings/datetime";
import Message from "../Message";
import {
    getExportEventTimestamp,
    getStreamId,
} from "../utils";
import ActionsHeader from "./ActionsHeader";


/**
 * Structure of search results data displayed in the table.
 */
interface SearchResult {
    _id: string;
    archive_id: string;
    dataset: string;
    filePath: string;
    log_event_ix: number;
    message: string;
    orig_file_id: string;
    orig_file_path: string;
    timestamp: number;
}

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
        timestamp: getExportEventTimestamp(result.timestamp),
        message: messageValue,
    });
};

/**
 * Cell renderer for the Actions column. Renders a per-row copy button.
 *
 * @param _
 * @param record
 * @return
 */
const renderActionsCell = (_: unknown, record: SearchResult) => {
    const handleCopyRow = () => {
        navigator.clipboard.writeText(formatResultAsJsonl(record))
            .catch((e: unknown) => {
                console.error("Failed to copy search result to clipboard", e);
            });
    };

    return (
        <Tooltip title={"Copy this event"}>
            <Button
                icon={<CopyOutlined/>}
                size={"small"}
                type={"text"}
                onClick={handleCopyRow}/>
        </Tooltip>
    );
};

const searchResultsTableColumns: NonNullable<TableProps<SearchResult>["columns"]> = [
    {
        dataIndex: "timestamp",
        defaultSortOrder: "descend",
        key: "timestamp",
        render: (timestamp: number) => formatTimestamp(timestamp),
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
        width: 12,
    },
    {
        dataIndex: "message",
        key: "message",
        render: (_, record) => (
            <Message
                dataset={record.dataset}
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
        width: 82,
    },
    {
        align: "right",
        key: "actions",
        render: renderActionsCell,
        title: <ActionsHeader/>,
        width: 6,
    },
];

export type {SearchResult};
export {
    formatResultAsJsonl,
    formatTimestamp,
    searchResultsTableColumns,
};
