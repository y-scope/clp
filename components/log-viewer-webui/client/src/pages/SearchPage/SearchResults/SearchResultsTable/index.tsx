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
import {useSearchResults} from "./useSearchResults";


/**
 * Renders search results in a table.
 *
 * @return
 */
const SearchResultsTable = () => {
    const {updateNumSearchResultsTable} = useSearchStore();
    const searchResults = useSearchResults();
    const [tableHeight, setTableHeight] = useState<number>(0);
    const containerRef = useRef<HTMLDivElement>(null);

    useEffect(() => {
        const num = searchResults ?
            searchResults.length :
            0;

        updateNumSearchResultsTable(num);
    }, [
        searchResults,
        updateNumSearchResultsTable,
    ]);

    // Antd table requires a fixed height for virtual scrolling. The effect sets a fixed height
    // based on the window height, container top, and fixed padding.
    useEffect(() => {
        const updateHeight = () => {
            if (containerRef.current) {
                const {top} = containerRef.current.getBoundingClientRect();
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
        <div
            ref={containerRef}
            style={{outline: "none"}}
        >
            <VirtualTable<SearchResult>
                columns={searchResultsTableColumns}
                dataSource={searchResults || []}
                pagination={false}
                rowKey={(record) => record._id.toString()}
                scroll={{y: tableHeight}}/>
        </div>
    );
};

export default SearchResultsTable;
