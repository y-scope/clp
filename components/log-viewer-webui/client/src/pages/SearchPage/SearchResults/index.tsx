import { Table } from "antd";
import { searchResultsColumns, SearchResult } from "./typings";

// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_RESULTS: SearchResult[] = [
    {
        id: 1,
        timestamp: "2023-01-01 12:00:00",
        message: "INFO: User login successful for user 'john.doe'.",
        filePath: "/var/logs/auth.log",
    },
    {
        id: 2,
        timestamp: "2023-01-01 12:01:00",
        message: "ERROR: Failed to connect to database 'logs_db'.",
        filePath: "/var/logs/db.log",
    },
    {
        id: 3,
        timestamp: "2023-01-01 12:02:00",
        message: "WARN: Disk space running low on volume '/var/logs'.",
        filePath: "/var/logs/system.log",
    },
    {
        id: 4,
        timestamp: "2023-01-01 12:03:00",
        message: "DEBUG: Processing request ID 12345.",
        filePath: "/var/logs/app.log",
    },
];

/**
 * Renders search results.
 *
 * @return
 */
const SearchResults = () => {
    return (
        <Table<SearchResult>
            columns={searchResultsColumns}
            dataSource={DUMMY_RESULTS}
            rowKey={(record) => record.id.toString()}
            virtual
            pagination={false}
        />
    );
};

export default SearchResults;
