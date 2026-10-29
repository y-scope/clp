import {DownloadOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import useSearchStore from "../../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";


/**
 * Column header component with an export-all button.
 *
 * @return
 */
const ActionsHeader = () => {
    const handleSearchResultsExport = useSearchStore((state) => state.handleSearchResultsExport);
    const numResults = useSearchStore((state) => state.numSearchResultsTable);
    const searchUiState = useSearchStore((state) => state.searchUiState);

    return (
        <Tooltip title={"Export all results as JSONL"}>
            <Button
                disabled={SEARCH_UI_STATE.DONE !== searchUiState || 0 === numResults}
                icon={<DownloadOutlined/>}
                size={"small"}
                type={"text"}
                onClick={handleSearchResultsExport}/>
        </Tooltip>
    );
};

export default ActionsHeader;
