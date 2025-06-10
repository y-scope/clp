import {
    useEffect,
    useRef,
    useState,
} from "react";

import {Table} from "antd";

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
        <div ref={containerRef}>
            <Table<SearchResult>
                columns={searchResultsTableColumns}
                pagination={false}
                rowKey={(record) => record._id.toString()}
                scroll={{y: tableHeight}}
                virtual={true}
                dataSource={searchResults ?
                    searchResults :
                    []}/>
        </div>
    );
};

export default SearchResultsTable;
