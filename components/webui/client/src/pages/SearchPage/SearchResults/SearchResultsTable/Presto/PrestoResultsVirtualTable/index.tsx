import {
    useEffect,
    useMemo,
} from "react";

import type {PrestoSearchResult} from "@webui/common";

import VirtualTable from "../../../../../../components/VirtualTable";
import useSearchStore from "../../../../SearchState/index";
import {usePrestoSearchResults} from "./usePrestoSearchResults";
import {getPrestoSearchResultsTableColumns} from "./utils";


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

    const columns = useMemo(
        () => getPrestoSearchResultsTableColumns(prestoSearchResults || []),
        [prestoSearchResults]
    );

    useEffect(() => {
        const num = prestoSearchResults ?
            prestoSearchResults.length :
            0;

        updateNumSearchResultsTable(num);
    }, [
        prestoSearchResults,
        updateNumSearchResultsTable,
    ]);

    return (
        <VirtualTable<PrestoSearchResult>
            columns={columns}
            dataSource={prestoSearchResults || []}
            pagination={false}
            rowKey={(record) => record._id}
            scroll={{y: tableHeight, x: "max-content"}}/>

    );
};

export default PrestoResultsVirtualTable;
