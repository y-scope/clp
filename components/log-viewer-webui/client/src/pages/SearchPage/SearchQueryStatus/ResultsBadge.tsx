import {useMemo} from "react";

import {
    Badge,
    ConfigProvider,
} from "antd";

import useSearchStore from "../SearchState/index";
import {SEARCH_UI_STATE} from "../SearchState/typings";
import styles from "./index.module.css";


/**
 * Displays the number of search results in a colored badge.
 *
 * @return
 */
const ResultsBadge = () => {
    const {
        numSearchResultsTimeline,
        numSearchResultsTable,
        searchUiState,
    } = useSearchStore();

    // Number of results is the maximum of the number of results in the timeline and table. The
    // timeline may have more results since the table results are capped. Having two sources may
    // be provide more timely update to the user.
    const numResults = useMemo(
        () => Math.max(numSearchResultsTimeline, numSearchResultsTable),
        [
            numSearchResultsTimeline,
            numSearchResultsTable,
        ]
    );

    return (

        <ConfigProvider
            theme={{
                components: {
                    Badge: {
                        indicatorHeight: 15,
                    },
                },
            }}
        >
            <Badge
                className={styles["badge"] || ""}
                count={numResults}
                overflowCount={Number.MAX_SAFE_INTEGER}
                showZero={true}
                color={
                    searchUiState === SEARCH_UI_STATE.QUERYING ||
                searchUiState === SEARCH_UI_STATE.CANCELLED ?
                        "yellow" :
                        "green"
                }/>
        </ConfigProvider>

    );
};

export default ResultsBadge;
