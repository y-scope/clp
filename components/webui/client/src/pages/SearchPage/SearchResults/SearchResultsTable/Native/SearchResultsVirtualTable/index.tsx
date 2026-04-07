import {
    useCallback,
    useEffect,
} from "react";

import {DownloadOutlined} from "@ant-design/icons";
import {
    Button,
    message,
    Tooltip,
} from "antd";
import dayjs from "dayjs";

import VirtualTable from "../../../../../../components/VirtualTable";
import {DATETIME_FORMAT_TEMPLATE} from "../../../../../../typings/datetime";
import useSearchStore from "../../../../SearchState/index";
import styles from "./index.module.css";
import {
    SearchResult,
    searchResultsTableColumns,
} from "./typings";
import {useSearchResults} from "./useSearchResults";


interface SearchResultsVirtualTableProps {
    tableHeight: number;
}

/**
 * Formats a search result as a "timestamp message" string.
 *
 * @param result
 * @return The formatted line.
 */
const formatResultLine = (result: SearchResult): string =>
    `${dayjs.utc(result.timestamp).format(DATETIME_FORMAT_TEMPLATE)} ${result.message}`;

/**
 * Renders search results in a virtual table with an export button.
 *
 * @param props
 * @param props.tableHeight
 * @return
 */
const SearchResultsVirtualTable = ({tableHeight}: SearchResultsVirtualTableProps) => {
    const {updateNumSearchResultsTable} = useSearchStore();
    const searchResults = useSearchResults();
    const [messageApi, contextHolder] = message.useMessage();

    useEffect(() => {
        const num = searchResults ?
            searchResults.length :
            0;

        updateNumSearchResultsTable(num);
    }, [
        searchResults,
        updateNumSearchResultsTable,
    ]);

    /**
     * Exports all search results as a text file download. Results are written
     * into a Blob incrementally to avoid building a single large string.
     */
    const handleExportAll = useCallback(() => {
        if (null === searchResults || 0 === searchResults.length) {
            return;
        }

        const parts: string[] = [];
        for (const result of searchResults) {
            parts.push(formatResultLine(result));
            parts.push("\n");
        }

        const blob = new Blob(parts, {type: "text/plain"});
        const url = URL.createObjectURL(blob);
        const anchor = document.createElement("a");
        anchor.href = url;
        anchor.download = "search-results.txt";
        anchor.click();
        URL.revokeObjectURL(url);

        messageApi.success(`Exported ${searchResults.length} results`);
    }, [
        searchResults,
        messageApi,
    ]);

    const hasResults = null !== searchResults && 0 < searchResults.length;

    return (
        <div>
            {contextHolder}
            {hasResults && (
                <div className={styles["exportButtonContainer"] || ""}>
                    <Tooltip title={"Export all results as a text file"}>
                        <Button
                            icon={<DownloadOutlined/>}
                            size={"small"}
                            onClick={handleExportAll}
                        >
                            {"Export All"}
                        </Button>
                    </Tooltip>
                </div>
            )}
            <VirtualTable<SearchResult>
                columns={searchResultsTableColumns}
                dataSource={searchResults || []}
                pagination={false}
                rowKey={(record) => record._id}
                scroll={{y: tableHeight, x: "max-content"}}/>
        </div>
    );
};

export default SearchResultsVirtualTable;
