import {useCallback} from "react";

import {SearchOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import {computeTimelineConfig} from "../../../SearchResults/SearchResultsTimeline/utils";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {handleQuerySubmit} from "../../search-requests";
import styles from "./index.module.css";


/**
 * Renders a button to submit the search query.
 *
 * @return
 */
const SubmitButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const timeRange = useSearchStore((state) => state.timeRange);
    const queryString = useSearchStore((state) => state.queryString);
    const selectDataset = useSearchStore((state) => state.selectDataset);
    const updateTimelineConfig = useSearchStore((state) => state.updateTimelineConfig);
    const updateCachedDataset = useSearchStore((state) => state.updateCachedDataset);

    let storageEngine = "clp-s";

    /**
     * Submits search query.
     */
    const handleSubmitButtonClick = useCallback(() => {
        // Update timeline to match range picker selection.
        const newTimelineConfig = computeTimelineConfig(timeRange);
        updateTimelineConfig(newTimelineConfig);

        if (storageEngine === "clp-s") {
            if (null !== selectDataset) {
                updateCachedDataset(selectDataset);
            } else {
                console.error("Cannot submit a clp-s query without a dataset selection.");
                return;
            }
        }

        handleQuerySubmit({
            dataset: selectDataset,
            ignoreCase: false,
            queryString: queryString,
            timeRangeBucketSizeMillis: newTimelineConfig.bucketDuration.asMilliseconds(),
            timestampBegin: timeRange[0].valueOf(),
            timestampEnd: timeRange[1].valueOf(),
        });
    }, [queryString, updateTimelineConfig, timeRange, selectDataset, updateCachedDataset]);

    const isQueryStringEmpty = queryString === SEARCH_STATE_DEFAULT.queryString;
    const isSomeDataset = selectDataset !== null && (storageEngine === "clp-s");

    let tooltipTitle = "";
    if (false === isSomeDataset) {
        tooltipTitle = "Data must be ingested prior to enable search";
    } else if (isQueryStringEmpty) {
        tooltipTitle = "Enter query to search";
    }

    return (
        <Tooltip title={tooltipTitle}>
            <Button
                className={styles["gradientButton"] || ""}
                disabled={isQueryStringEmpty || false === isSomeDataset ||
                          searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING}
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
