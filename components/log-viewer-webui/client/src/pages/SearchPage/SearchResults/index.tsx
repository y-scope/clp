import { Table } from "antd";
import { jobColumns, SearchResult } from "./typings";

const SearchResults = () => {
    const data: SearchResult[] = [
        {
            id: 1,
            timestamp: "2023-01-01 12:00:00",
            message: "INFO: User login successful for user 'john.doe'.",
            originalFilePath: "/var/logs/auth.log",
        },
        {
            id: 2,
            timestamp: "2023-01-01 12:01:00",
            message: "ERROR: Failed to connect to database 'logs_db'.",
            originalFilePath: "/var/logs/db.log",
        },
        {
            id: 3,
            timestamp: "2023-01-01 12:02:00",
            message: "WARN: Disk space running low on volume '/var/logs'.",
            originalFilePath: "/var/logs/system.log",
        },
        {
            id: 4,
            timestamp: "2023-01-01 12:03:00",
            message: "DEBUG: Processing request ID 12345.",
            originalFilePath: "/var/logs/app.log",
        },
    ];

    return (
        <Table<SearchResult>
            columns={jobColumns}
            dataSource={data}
            rowKey={(record) => record.id.toString()}
            virtual
            pagination={false}
        />
    );
};

export default SearchResults;
