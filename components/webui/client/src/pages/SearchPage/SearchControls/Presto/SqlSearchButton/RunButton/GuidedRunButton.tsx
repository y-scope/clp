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
            const trimmedWhere = where.trim();
            const trimmedOrderBy = orderBy.trim();
            const limitString = String(limit);

            const queryString = buildSearchQuery({
                selectItemList: select.trim(),
                databaseName: from,
                startTimestamp: startTimestamp.unix(),
                endTimestamp: endTimestamp.unix(),
                timestampKey: timestampKey,
                ...(trimmedWhere && { booleanExpression: trimmedWhere }),
                ...(trimmedOrderBy && { sortItemList: trimmedOrderBy }),
                ...(limitString && { limitValue: limitString }),
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
