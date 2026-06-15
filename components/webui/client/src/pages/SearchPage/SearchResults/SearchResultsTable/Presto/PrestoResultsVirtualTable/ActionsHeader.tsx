import {DownloadOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import useSearchStore from "../../../../SearchState/index";
import usePrestoSearchState from "../../../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";


/**
 * Column header component with an export-all button for Presto results.
 *
 * @return The export button wrapped in a tooltip.
 */
const ActionsHeader = () => {
    const handlePrestoSearchResultsExport = usePrestoSearchState(
        (state) => state.handlePrestoSearchResultsExport
    );
    const numResults = useSearchStore((state) => state.numSearchResultsTable);
    const searchUiState = useSearchStore((state) => state.searchUiState);

    return (
        <Tooltip title={"Export all results as JSONL"}>
            <Button
                disabled={SEARCH_UI_STATE.DONE !== searchUiState || 0 === numResults}
                icon={<DownloadOutlined/>}
                size={"small"}
                type={"text"}
                onClick={handlePrestoSearchResultsExport}/>
        </Tooltip>
    );
};

export default ActionsHeader;
