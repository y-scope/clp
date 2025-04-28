import { TableProps } from "antd";
import Message from "./Message"; // Updated import path

interface SearchResult {
    id: number;
    timestamp: string;
    message: string;
    filePath: string;
}

const searchResultsColumns: NonNullable<TableProps<SearchResult>["columns"]> = [
    {
        title: "Timestamp",
        dataIndex: "timestamp",
        key: "timestamp",
        sorter: true,
        width: "15%",
    },
    {
        title: "Message",
        dataIndex: "message",
        key: "message",
        width: "85%",
        render: (_, record) => <Message message={record.message} filePath={record.filePath} />,
    },
];

export type { SearchResult };
export { searchResultsColumns };