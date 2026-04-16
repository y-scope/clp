import {useEffect} from "react";

import VirtualTable from "../../../../../../components/VirtualTable";
import useSearchStore from "../../../../SearchState/index";
import {
    SearchResult,
    searchResultsTableColumns,
} from "./typings";
import {useSearchResults} from "./useSearchResults";


interface SearchResultsVirtualTableProps {
    tableHeight: number;
}

/**
 * Renders search results in a virtual table and syncs results to the search store
 * so the export action can read them without a duplicate cursor subscription.
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
        updateNumSearchResultsTable,
        updateSearchResults,
    } = useSearchStore();
    const searchResults = useSearchResults();

    useEffect(() => {
        const num = searchResults ?
            searchResults.length :
            0;

        updateNumSearchResultsTable(num);
        updateSearchResults(searchResults);
    }, [
        searchResults,
        updateNumSearchResultsTable,
        updateSearchResults,
    ]);

    return (
        <VirtualTable<SearchResult>
            columns={searchResultsTableColumns}
            dataSource={searchResults || []}
            pagination={false}
            rowKey={(record) => record._id}
            scroll={{y: tableHeight}}/>
    );
};

export default SearchResultsVirtualTable;