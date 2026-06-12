import {
    useEffect,
    useMemo,
} from "react";

import type {PrestoSearchResult} from "@webui/common/presto";

import VirtualTable from "../../../../../../components/VirtualTable";
import useSearchStore from "../../../../SearchState/index";
import usePrestoSearchState from "../../../../SearchState/Presto";
import ActionsHeader from "./ActionsHeader";
import {usePrestoSearchResults} from "./usePrestoSearchResults";
import {getPrestoSearchResultsTableColumns} from "./utils";


const PRESTO_SEARCH_RESULTS_ACTIONS_COLUMN = {
    align: "right" as const,
    key: "actions",
    title: <ActionsHeader/>,
    width: 6,
};


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
    const {updatePrestoSearchResults} = usePrestoSearchState();
    const prestoSearchResults = usePrestoSearchResults();

    const columns = useMemo(
        () => [
            ...getPrestoSearchResultsTableColumns(prestoSearchResults || []),
            PRESTO_SEARCH_RESULTS_ACTIONS_COLUMN,
        ],
        [prestoSearchResults]
    );

    useEffect(() => {
        const num = prestoSearchResults ?
            prestoSearchResults.length :
            0;

        updateNumSearchResultsTable(num);
        updatePrestoSearchResults(prestoSearchResults);
    }, [
        prestoSearchResults,
        updateNumSearchResultsTable,
        updatePrestoSearchResults,
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
