import {TableProps} from "antd";


/**
 * Structure of dynamic Presto search results data.
 */
interface PrestoSearchResult {
    _id: string;
    row: Record<string, unknown>;
}

/**
 * Generates dynamic columns configuration for Presto query engine.
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
