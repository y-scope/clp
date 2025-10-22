import {EditOutlined} from "@ant-design/icons";
import {Button} from "antd";

import useSearchStore from "../../../../SearchState/index";
import usePrestoSearchState from "../../../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../../../SearchState/Presto/typings";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";


/**
 * Renders a button to switch to Freeform SQL interface.
 *
 * @return
 */
const FreeformButton = () => {
    const setSqlInterface = usePrestoSearchState((state) => state.setSqlInterface);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    const handleClick = () => {
        setSqlInterface(PRESTO_SQL_INTERFACE.FREEFORM);
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
