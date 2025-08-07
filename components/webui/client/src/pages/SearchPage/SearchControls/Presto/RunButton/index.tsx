import {useCallback} from "react";

import {
    CaretRightOutlined,
    CloseOutlined,
} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import useSearchStore from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {
    handlePrestoQueryCancel,
    handlePrestoQuerySubmit,
} from "../presto-search-requests";
import styles from "./index.module.css";


/**
 * Renders a button to run the SQL query.
 *
 * @return
 */
const RunButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const queryString = useSearchStore((state) => state.queryString);
    const searchJobId = useSearchStore((state) => state.searchJobId);

    const isQueryStringEmpty = "" === queryString;
    const tooltipTitle = isQueryStringEmpty ?
        "Enter SQL query to run" :
        "";

    const handleClick = useCallback(() => {
        handlePrestoQuerySubmit({queryString});
    }, [queryString]);

    const handleCancel = useCallback(() => {
        if (null === searchJobId) {
            console.error("Cannot cancel query: searchJobId is not set.");

            return;
        }
        handlePrestoQueryCancel({searchJobId});
    }, [searchJobId]);

    return (
        <div className={styles["runButtonContainer"] || ""}>
            { (searchUiState === SEARCH_UI_STATE.QUERYING) ?

                <Tooltip title={"Cancel query"}>
                    <Button
                        className={styles["cancelButton"] || ""}
                        color={"red"}
                        disabled={isQueryStringEmpty}
                        icon={<CloseOutlined/>}
                        size={"large"}
                        variant={"solid"}
                        onClick={handleCancel}
                    >
                        Cancel
                    </Button>
                </Tooltip> :

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
                </Tooltip>}
        </div>
    );
};

export default RunButton;
