import {AppstoreOutlined} from "@ant-design/icons";
import {Button} from "antd";

import useSearchStore from "../../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {handleSwitchToGuided} from "../../presto-search-requests";


/**
 * Renders a button to switch to Guided SQL interface.
 *
 * @return
 */
const GuidedButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    const handleClick = () => {
        handleSwitchToGuided();
    };

    return (
        <Button
            block={true}
            color={"default"}
            disabled={disabled}
            icon={<AppstoreOutlined/>}
            size={"middle"}
            variant={"filled"}
            onClick={handleClick}
        >
            Guided
        </Button>
    );
};

export default GuidedButton;
