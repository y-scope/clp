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
        title: "Timestamp",
        sorter: true,
        width: 15,
    },
    {
        dataIndex: "message",
        key: "message",
        title: "Message",
        render: (_, record) => (
            <Message
                filePath={record.filePath}
                message={record.message}/>
        ),
        width: 85,
    },
];

export type {SearchResult};
export {searchResultsTableColumns};
