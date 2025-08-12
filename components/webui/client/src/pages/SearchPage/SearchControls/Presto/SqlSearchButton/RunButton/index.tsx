import {useCallback} from "react";

import {CaretRightOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import useSearchStore from "../../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {handlePrestoQuerySubmit} from "../../presto-search-requests";
import styles from "./index.module.css";


/**
 * Renders a button to run the SQL query.
 *
 * @return
 */
const RunButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const queryString = useSearchStore((state) => state.queryString);

    const isQueryStringEmpty = "" === queryString;
    const tooltipTitle = isQueryStringEmpty ?
        "Enter SQL query to run" :
        "";

    const handleClick = useCallback(() => {
        handlePrestoQuerySubmit({queryString});
    }, [queryString]);

    return (
        <Tooltip title={tooltipTitle}>
            <Button
                className={styles["runButton"] || ""}
                color={"green"}
                icon={<CaretRightOutlined/>}
                size={"large"}
                variant={"solid"}
                disabled={isQueryStringEmpty ||
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING}
                onClick={handleClick}
            >
                Run
            </Button>
        </Tooltip>
    );
};

export default RunButton;
