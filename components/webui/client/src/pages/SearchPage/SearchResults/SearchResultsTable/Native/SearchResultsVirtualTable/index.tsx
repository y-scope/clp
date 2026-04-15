import {
    useCallback,
    useEffect,
} from "react";

import {message} from "antd";

import VirtualTable from "../../../../../../components/VirtualTable";
import {downloadTextFile} from "../../../../../../utils/download";
import useSearchStore from "../../../../SearchState/index";
import {getExportFilenameTimestamp} from "../utils";
import {
    formatResultAsJsonl,
    SearchResult,
    searchResultsTableColumns,
} from "./typings";
import {useSearchResults} from "./useSearchResults";


interface SearchResultsVirtualTableProps {
    tableHeight: number;
}

/**
 * Renders search results in a virtual table and registers an export handler
 * with the search store so the table-header export button can trigger a download
 * without a duplicate cursor subscription.
 *
 * NOTE: Export is currently only available for the Native search engine. The
 * Presto engine's PrestoResultsVirtualTable does not yet support export.
 *
 * @param props
 * @param props.tableHeight
 * @return
 */
const SearchResultsVirtualTable = ({tableHeight}: SearchResultsVirtualTableProps) => {
    const {
        setOnSearchResultsExport,
        updateNumSearchResultsTable,
    } = useSearchStore();
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
     * Exports all search results as a JSONL file download.
     *
     * NOTE: Results are exported in the original cursor order (i.e., timestamp descending),
     * which may differ from the user's current table sort.
     */
    const handleExport = useCallback(() => {
        if (null === searchResults || 0 === searchResults.length) {
            return;
        }

        try {
            downloadTextFile(
                searchResults.map((r) => `${formatResultAsJsonl(r)}\n`),
                `clp-search-results-${getExportFilenameTimestamp()}.jsonl`
            );
            messageApi.success(`Exported ${searchResults.length} results`);
        } catch (e) {
            messageApi.error("Failed to export results");
            console.error(e);
        }
    }, [
        messageApi,
        searchResults,
    ]);

    useEffect(() => {
        setOnSearchResultsExport(handleExport);

        return () => {
            setOnSearchResultsExport(null);
        };
    }, [
        handleExport,
        setOnSearchResultsExport,
    ]);

    return (
        <>
            {contextHolder}
            <VirtualTable<SearchResult>
                columns={searchResultsTableColumns}
                dataSource={searchResults || []}
                pagination={false}
                rowKey={(record) => record._id}
                scroll={{y: tableHeight}}/>
        </>
    );
};

export default SearchResultsVirtualTable;
