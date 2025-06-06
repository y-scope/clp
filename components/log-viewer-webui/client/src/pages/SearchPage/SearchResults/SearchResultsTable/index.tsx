import {
    useEffect,
    useRef,
    useState,
} from "react";
import VirtualTable from "../../../../components/VirtualTable";

import useSearchStore from "../../SearchState/index";
import {
    SearchResult,
    searchResultsTableColumns,
    TABLE_BOTTOM_PADDING,
} from "./typings";
import { useSearchResults } from "./useSearchResults";

/**
 * Renders search results in a table.
 *
 * @return
 */
const SearchResultsTable = () => {
    const { updateNumSearchResultsTable } = useSearchStore();
    const searchResults = useSearchResults();
    const [tableHeight, setTableHeight] = useState<number>(0);
    const tableRef = useRef<any>(null);

    useEffect(() => {
        const num = searchResults ? searchResults.length : 0;
        updateNumSearchResultsTable(num);
    }, [searchResults, updateNumSearchResultsTable]);

    useEffect(() => {
        const updateHeight = () => {
            if (tableRef.current?.nativeElement) {
                const tableElement = tableRef.current.nativeElement;
                const { top } = tableElement.getBoundingClientRect();
                const availableHeight = window.innerHeight - top - TABLE_BOTTOM_PADDING;
                setTableHeight(availableHeight);
            }
        };

        updateHeight();
        window.addEventListener("resize", updateHeight);

        return () => {
            window.removeEventListener("resize", updateHeight);
        };
    }, []);

    return (
        <VirtualTable<SearchResult>
            ref={tableRef}
            columns={searchResultsTableColumns}
            rowKey={(record) => record._id.toString()}
            scrollY={tableHeight}
            dataSource={searchResults || []}
            pagination={false}
        />
    );
};

export default SearchResultsTable;
