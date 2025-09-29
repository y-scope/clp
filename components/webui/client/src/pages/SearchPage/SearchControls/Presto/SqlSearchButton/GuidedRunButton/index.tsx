import {useCallback} from "react";

import {CaretRightOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import {buildSearchQuery} from "../../../../../../sql-parser";
import usePrestoSearchState from "../../../../SearchState/Presto";
import useSearchStore from "../../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {handlePrestoQuerySubmit} from "../../presto-search-requests";
import styles from "../RunButton/index.module.css";


/**
 * Renders a button to run the guided SQL query.
 *
 * @return
 */
const GuidedRunButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);

    const from = usePrestoSearchState((state) => state.from);
    const timestampKey = usePrestoSearchState((state) => state.timestampKey);
    const select = usePrestoSearchState((state) => state.select);

    const isQueryReady = null !== from && null !== timestampKey && "" !== select.trim();
    const tooltipTitle = !isQueryReady ?
        "Select database and timestamp key" :
        "";

    const handleClick = useCallback(() => {
        const {select, from, where, orderBy, limit, timestampKey} = usePrestoSearchState.getState();
        const {timeRange} = useSearchStore.getState();
        const [startTimestamp, endTimestamp] = timeRange;

        if (null === from) {
            console.error("Cannot build guided query: from input is missing");
            return;
        }

        if (null === timestampKey) {
            console.error("Cannot build guided query: timestampKey input is missing");
            return;
        }

        try {
            const finalQueryString = buildSearchQuery({
                selectItemList: select.trim() || "*",
                databaseName: from,
                booleanExpression: where.trim(),
                sortItemList: orderBy.trim(),
                limitValue: limit,
                startTimestamp: startTimestamp.unix(),
                endTimestamp: endTimestamp.unix(),
                timestampKey,
            });

            handlePrestoQuerySubmit({queryString: finalQueryString});
        } catch (err) {
            console.error("Failed to build guided query:", err);
        }
    }, []);

    return (
        <Tooltip title={tooltipTitle}>
            <Button
                className={styles["runButton"] || ""}
                color={"green"}
                icon={<CaretRightOutlined/>}
                size={"middle"}
                variant={"solid"}
                disabled={!isQueryReady ||
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING}
                onClick={handleClick}
            >
                Run
            </Button>
        </Tooltip>
    );
};

export default GuidedRunButton;
