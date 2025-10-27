import {EditOutlined} from "@ant-design/icons";
import {Button} from "antd";

import useSearchStore from "../../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {handleSwitchToFreeform} from "../../presto-guided-search-requests";


/**
 * Renders a button to switch to Freeform SQL interface.
 *
 * @return
 */
const FreeformButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    const handleClick = () => {
        handleSwitchToFreeform();
    };

    return (
        <Button
            block={true}
            color={"default"}
            disabled={disabled}
            icon={<EditOutlined/>}
            size={"middle"}
            variant={"filled"}
            onClick={handleClick}
        >
            Freeform
        </Button>
    );
};


export default FreeformButton;
