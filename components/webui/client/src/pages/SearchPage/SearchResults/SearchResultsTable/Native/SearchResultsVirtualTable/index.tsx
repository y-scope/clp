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
 * Renders search results in a virtual table.
 *
 * @param props
 * @param props.tableHeight
 * @return
 */
const SearchResultsVirtualTable = ({tableHeight}: SearchResultsVirtualTableProps) => {
    const {updateNumSearchResultsTable} = useSearchStore();
    const searchResults = useSearchResults();

    useEffect(() => {
        const num = searchResults ?
            searchResults.length :
            0;

        updateNumSearchResultsTable(num);
    }, [
        searchResults,
        updateNumSearchResultsTable,
    ]);

    return (
        <VirtualTable<SearchResult>
            columns={searchResultsTableColumns}
            dataSource={searchResults || []}
            pagination={false}
            rowKey={(record) => record._id}
            scroll={{y: tableHeight, x: "max-content"}}/>
    );
};

export default SearchResultsVirtualTable;
