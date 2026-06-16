import type {PrestoSearchResult} from "@webui/common/presto";
import {TableProps} from "antd";


/**
 * Prefix reserved for synthetic/derived columns in Presto tables.
 */
const RESULT_COLUMN_KEY_PREFIX = "result-col-";

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
            key: `${RESULT_COLUMN_KEY_PREFIX}${key}`,
            title: key,
            width: 100,
        }));
};

/**
 * Serializes a Presto search result as a JSONL line by exporting the row object directly. Since
 * Presto columns are user-defined based on the SQL query, this keeps the export payload aligned
 * with query columns instead of selecting fixed fields.
 *
 * If the row is missing/invalid, an empty JSON object is returned.
 *
 * @param result Presto search result
 * @return A single JSON line (without trailing newline)
 */
const formatPrestoResultAsJsonl = (result: PrestoSearchResult): string => {
    if ("undefined" === typeof result.row) {
        return JSON.stringify({});
    }

    return JSON.stringify(row as Record<string, unknown>);
};

export {
    formatPrestoResultAsJsonl,
    getPrestoSearchResultsTableColumns,
};
