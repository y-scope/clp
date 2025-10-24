import {AppstoreOutlined} from "@ant-design/icons";
import {Button} from "antd";

import useSearchStore from "../../../../SearchState/index";
import usePrestoSearchState from "../../../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../../../SearchState/Presto/typings";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";


/**
 * Renders a button to switch to Guided SQL interface.
 *
 * @return
 */
const GuidedButton = () => {
    const setSqlInterface = usePrestoSearchState((state) => state.setSqlInterface);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    const handleClick = () => {
        setSqlInterface(PRESTO_SQL_INTERFACE.GUIDED);
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
