import type {PrestoSearchResult} from "@webui/common/presto";
import {TableProps} from "antd";


/**
 * Generates dynamic columns configuration for Presto query engine.
 *
 * @param data Array of Presto search results
 * @return
 */
const getPrestoSearchResultsTableColumns = (
    data: PrestoSearchResult[]
): NonNullable<TableProps<PrestoSearchResult>["columns"]> => {
    if (0 === data.length ||
        "undefined" === typeof data[0] ||
        "undefined" === typeof data[0].row
    ) {
        return [];
    }

    return Object.keys(data[0].row)
        .map((key) => ({
            dataIndex: [
                "row",
                key,
            ],
            key: key,
            title: key,
            width: 100,
        }));
};

/**
 * Serializes a Presto search result as a JSONL line by exporting the row
 * object directly. Since Presto columns are user-defined based on the SQL
 * query, the entire row is serialized rather than specific fields.
 *
 * @param result Presto search result with _id and row properties
 * @return A single JSON line (without trailing newline)
 */
const formatPrestoResultAsJsonl = (result: PrestoSearchResult): string => {
    return JSON.stringify(result.row);
};

export {
    formatPrestoResultAsJsonl,
    getPrestoSearchResultsTableColumns,
};
