import {TableProps} from "antd";


/**
 * Structure of Presto search results data (dynamic properties).
 */
interface PrestoSearchResult {
    _id: string;
    row: Record<string, unknown>;
}

/**
 * Columns configuration for Presto query engine (dynamic based on data).
 *
 * @param data Array of Presto search results
 * @return
 */
const getPrestoSearchResultsTableColumns = (
    data: PrestoSearchResult[]
): NonNullable<TableProps<PrestoSearchResult>["columns"]> => {
    if (0 === data.length || "undefined" === typeof data[0] || "undefined" === typeof data[0].row) {
        return [];
    }

    return Object.keys(data[0].row)
        .map((key) => ({
            dataIndex: ["row",
                key],
            key: key,
            title: key,
        }));
};

export type {PrestoSearchResult};
export {getPrestoSearchResultsTableColumns};
