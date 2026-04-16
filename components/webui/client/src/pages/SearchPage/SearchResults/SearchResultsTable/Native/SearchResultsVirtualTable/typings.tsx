import {CopyOutlined} from "@ant-design/icons";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {
    Button,
    message,
    TableProps,
    Tooltip,
} from "antd";

import {SETTINGS_STORAGE_ENGINE} from "../../../../../../config";
import Message from "../Message";
import {
    formatResultAsJsonl,
    formatTimestamp,
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
                const errMsg = "Failed to copy event to clipboard.";
                message.error(errMsg);
                console.error(errMsg, e);
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
