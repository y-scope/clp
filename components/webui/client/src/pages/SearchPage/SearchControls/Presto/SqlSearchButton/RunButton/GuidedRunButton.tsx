import {useCallback} from "react";

import {CaretRightOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import {buildSearchQuery} from "../../../../../../sql-parser";
import useSearchStore from "../../../../SearchState/index";
import usePrestoSearchState from "../../../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {handlePrestoQuerySubmit} from "../../presto-search-requests";
import styles from "./index.module.css";


/**
 * Renders a button to run the guided SQL query.
 *
 * @return
 */
const GuidedRunButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const timeRange = useSearchStore((state) => state.timeRange);
    const [startTimestamp, endTimestamp] = timeRange;

    const {select, from, where, orderBy, limit, timestampKey} = usePrestoSearchState();

    const isQueryReady =
        "" !== select.trim() &&
        null !== from &&
        null !== timestampKey;

    const tooltipTitle = false === isQueryReady ?
        "Enter minimal SQL fields to query" :
        "";

    const handleClick = useCallback(() => {
        if (null === from) {
            console.error("Cannot build guided query: from input is missing");

            return;
        }

        if (null === timestampKey) {
            console.error("Cannot build guided query: timestampKey input is missing");

            return;
        }

        try {
            // eslint-disable-next-line no-undefined
            const getUndefinedIfEmpty = (str: string) => str === "" ? undefined : str;
            const queryString = buildSearchQuery({
                booleanExpression: getUndefinedIfEmpty(where.trim()),
                databaseName: from,
                endTimestamp: endTimestamp.unix(),
                limitValue: String(limit),
                selectItemList: select.trim(),
                sortItemList: getUndefinedIfEmpty(orderBy.trim()),
                startTimestamp: startTimestamp.unix(),
                timestampKey: timestampKey,
            });

            handlePrestoQuerySubmit({queryString});
        } catch (err) {
            console.error("Failed to build guided query:", err);
        }
    }, [
        select,
        from,
        where,
        orderBy,
        limit,
        timestampKey,
        startTimestamp,
        endTimestamp,
    ]);

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
