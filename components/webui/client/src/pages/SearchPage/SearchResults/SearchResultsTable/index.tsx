import {
    useEffect,
    useRef,
    useState,
} from "react";

import {CLP_QUERY_ENGINES} from "@webui/common/config";

import {SETTINGS_QUERY_ENGINE} from "../../../../config";
import usePrestoSearchState from "../../SearchState/Presto";
import SearchResultsVirtualTable from "./Native/SearchResultsVirtualTable";
import PrestoResultsVirtualTable from "./Presto/PrestoResultsVirtualTable";
import {TABLE_BOTTOM_PADDING} from "./typings";


/**
 * Renders search results in a table.
 *
 * @return
 */
const SearchResultsTable = () => {
    const [tableHeight, setTableHeight] = useState<number>(0);
    const containerRef = useRef<HTMLDivElement>(null);
    const {sqlInterface} = usePrestoSearchState();

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
    }, [sqlInterface]);

    return (
        <div
            ref={containerRef}
            style={{outline: "none"}}
        >
            {CLP_QUERY_ENGINES.PRESTO === SETTINGS_QUERY_ENGINE ?
                (
                    <PrestoResultsVirtualTable tableHeight={tableHeight}/>
                ) :
                (
                    <SearchResultsVirtualTable tableHeight={tableHeight}/>
                )}
        </div>
    );
};

export default SearchResultsTable;
