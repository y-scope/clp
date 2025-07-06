import {useCallback} from "react";

import {SearchOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import {computeTimelineConfig} from "../../SearchResults/SearchResultsTimeline/utils";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import {handleQuerySubmit} from "../search-requests";
import styles from "./index.module.css";


/**
 * Renders a button to submit the search query.
 *
 * @return
 */
const SubmitButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const queryString = useSearchStore((state) => state.queryString);
    const queryIsCaseSensitive = useSearchStore((state) => state.queryIsCaseSensitive);
    const timeRange = useSearchStore((state) => state.timeRange);

    /**
     * Submits search query.
     */
    const handleSubmitButtonClick = useCallback(() => {
        // Update timeline to match range picker selection.
        const newTimelineConfig = computeTimelineConfig(timeRange);
        const {updateTimelineConfig} = useSearchStore.getState();
        updateTimelineConfig(newTimelineConfig);

        handleQuerySubmit({
            ignoreCase: false === queryIsCaseSensitive,
            queryString: queryString,
            timeRangeBucketSizeMillis: newTimelineConfig.bucketDuration.asMilliseconds(),
            timestampBegin: timeRange[0].valueOf(),
            timestampEnd: timeRange[1].valueOf(),
        });
    }, [queryString,
        queryIsCaseSensitive,
        timeRange]);

    const isQueryStringEmpty = queryString === SEARCH_STATE_DEFAULT.queryString;

    return (
        <Tooltip
            title={isQueryStringEmpty ?
                "Enter query to search" :
                ""}
        >
            <Button
                className={styles["gradientButton"] || ""}
                disabled={isQueryStringEmpty || searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING}
                htmlType={"submit"}
                icon={<SearchOutlined/>}
                size={"large"}
                type={"primary"}
                onClick={handleSubmitButtonClick}
            >
                Search
            </Button>
        </Tooltip>
    );
};

export default SubmitButton;
