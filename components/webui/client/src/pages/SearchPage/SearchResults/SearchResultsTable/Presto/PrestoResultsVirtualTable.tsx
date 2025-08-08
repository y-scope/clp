import {useEffect} from "react";

import VirtualTable from "../../../../../components/VirtualTable";
import useSearchStore from "../../../SearchState/index";
import {
    getPrestoSearchResultsTableColumns,
    PrestoSearchResult,
} from "./typings";
import {usePrestoSearchResults} from "./usePrestoSearchResults";


interface PrestoResultsVirtualTableProps {
    tableHeight: number;
}

/**
 * Renders Presto search results in a virtual table.
 *
 * @param props
 * @param props.tableHeight
 * @return
 */
const PrestoResultsVirtualTable = ({tableHeight}: PrestoResultsVirtualTableProps) => {
    const {updateNumSearchResultsTable} = useSearchStore();
    const prestoSearchResults = usePrestoSearchResults();

    useEffect(() => {
        const num = prestoSearchResults ?
            prestoSearchResults.length :
            0;

        updateNumSearchResultsTable(num);
    }, [prestoSearchResults,
        updateNumSearchResultsTable]);

    return (
        <VirtualTable<PrestoSearchResult>
            columns={getPrestoSearchResultsTableColumns(prestoSearchResults || [])}
            dataSource={prestoSearchResults || []}
            pagination={false}
            rowKey={(record) => record._id}
            scroll={{y: tableHeight}}/>
    );
};

export default PrestoResultsVirtualTable;
