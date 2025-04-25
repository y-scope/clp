import { TableProps } from "antd";
import Message from "./Message"; // Updated import path

interface SearchResult {
    id: number;
    timestamp: string;
    message: string;
    originalFilePath: string;
}

const jobColumns: NonNullable<TableProps<SearchResult>["columns"]> = [
    {
        title: "Timestamp",
        dataIndex: "timestamp",
        key: "timestamp",
        sorter: true,
        width: "20%",
    },
    {
        title: "Message",
        dataIndex: "message",
        key: "message",
        width: "80%",
        render: (_, record) => <Message message={record.message} filePath={record.originalFilePath} />,
    },
];

export type { SearchResult };
export { jobColumns };