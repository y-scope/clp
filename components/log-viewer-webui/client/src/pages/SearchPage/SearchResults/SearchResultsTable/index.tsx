import {
    useEffect,
    useRef,
    useState,
} from "react";

import VirtualTable from "../../../../components/VirtualTable";
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
    const searchResults = useSearchResults();
    const [tableHeight, setTableHeight] = useState<number>(0);
    const containerRef = useRef<HTMLDivElement>(null);

    useEffect(() => {
        const updateHeight = () => {
            if (containerRef.current) {
                const { top } = containerRef.current.getBoundingClientRect();
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
        <div ref={containerRef} style={{outline: "none"}}>
            <VirtualTable<SearchResult>
                columns={searchResultsTableColumns}
                dataSource={searchResults || []}
                pagination={false}
                rowKey={(record) => record._id.toString()}
                scroll={{ y: tableHeight }}
            />
        </div>
    );
};

export default SearchResultsTable;
