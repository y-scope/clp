import {TableProps} from "antd";

import {PRESTO_DATA_PROPERTY} from "../../../../../../../../common";
import {PrestoSearchResult} from "./typings";


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
        "undefined" === typeof data[0][PRESTO_DATA_PROPERTY]
    ) {
        return [];
    }

    return Object.keys(data[0].row)
        .map((key) => ({
            dataIndex: [
                PRESTO_DATA_PROPERTY,
                key,
            ],
            key: key,
            title: key,
        }));
};

export {getPrestoSearchResultsTableColumns};
