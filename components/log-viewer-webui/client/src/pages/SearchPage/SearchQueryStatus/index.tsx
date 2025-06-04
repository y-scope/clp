import {
    useMemo
} from "react";
import useSearchStore from "../SearchState/index";
import { Badge, Typography, ConfigProvider } from "antd";
import styles from "./index.module.css"
import { SEARCH_UI_STATE } from "../SearchState/typings";


const SearchQueryStatus = () => {
    const {numSearchResultsTimeline, numSearchResultsTable, searchJobId, searchUiState} = useSearchStore();

    const numResults = useMemo(
        () => Math.max(numSearchResultsTimeline, numSearchResultsTable),
        [numSearchResultsTimeline, numSearchResultsTable]
    );

    return (
        <div className={styles["status"]}>
            {searchUiState !== SEARCH_UI_STATE.DEFAULT && (
                <Typography.Text type="secondary">
                    Search job #{searchJobId} found{" "}
                </Typography.Text>
            )}
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
                    count={numResults}
                    color={
                        searchUiState === SEARCH_UI_STATE.QUERYING ||
                        searchUiState === SEARCH_UI_STATE.CANCELLED
                            ? "yellow"
                            : "green"
                    }
                    showZero
                    className={styles["badge"] || ""}
                    overflowCount={Number.MAX_SAFE_INTEGER}
                />
            </ConfigProvider>
            <Typography.Text type="secondary"> results</Typography.Text>
        </div>
    );
};


export default SearchQueryStatus;
