import {TableProps} from "antd";

import Message from "./Message";


/**
 * Structure of search results data displayed in the table.
 */
interface SearchResult {
    id: number;
    timestamp: string;
    message: string;
    filePath: string;
}

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
    },
    {
        dataIndex: "message",
        key: "message",
        render: (_, record) => (
            <Message
                filePath={record.filePath}
                message={record.message}/>
        ),
        title: "Message",
        width: 85,
    },
];

export type {SearchResult};
export {searchResultsTableColumns};
