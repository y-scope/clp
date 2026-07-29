import {EyeOutlined} from "@ant-design/icons";
import {
    Button,
    theme,
    Tooltip,
} from "antd";

import useSearchStore from "../../SearchState";
import usePrestoSearchState from "../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../SearchState/typings";


/**
 * Renders a button that opens the query drawer.
 *
 * @return
 */
const OpenQueryDrawerButton = () => {
    const {token} = theme.useToken();
    const updateQueryDrawerOpen = usePrestoSearchState((s) => s.updateQueryDrawerOpen);
    const searchUiState = useSearchStore((s) => s.searchUiState);

    const tooltipText =
        searchUiState === SEARCH_UI_STATE.FAILED ?
            "See modified query and error" :
            "See modified query";

    return (
        <Tooltip title={tooltipText}>
            <Button
                color={"default"}
                size={"small"}
                variant={"filled"}
                icon={
                    <EyeOutlined
                        style={{
                            color: token.colorTextSecondary,
                        }}/>
                }
                onClick={() => {
                    updateQueryDrawerOpen(true);
                }}/>
        </Tooltip>
    );
};

export default OpenQueryDrawerButton;
