import {useCallback} from "react";

import {SearchOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../../../config";
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

    /**
     * Submits search query.
     */
    const handleSubmitButtonClick = useCallback(() => {
        // Update timeline to match range picker selection.
        const newTimelineConfig = computeTimelineConfig(timeRange);
        updateTimelineConfig(newTimelineConfig);

        if (CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE) {
            if (null !== selectDataset) {
                updateCachedDataset(selectDataset);
            } else {
                console.error("Cannot submit a clp-s query without a dataset selection.");

                return;
            }
        }

        handleQuerySubmit({
            dataset: selectDataset ?? "",
            ignoreCase: false,
            queryString: queryString,
            timeRangeBucketSizeMillis: newTimelineConfig.bucketDuration.asMilliseconds(),
            timestampBegin: timeRange[0].valueOf(),
            timestampEnd: timeRange[1].valueOf(),
        });
    }, [queryString,
        updateTimelineConfig,
        timeRange,
        selectDataset,
        updateCachedDataset]);

    const isQueryStringEmpty = queryString === SEARCH_STATE_DEFAULT.queryString;

    // Submit button must be disabled if there are no datasets since clp-s requires dataset option
    // for queries.
    const isSomeDataset = (null !== selectDataset) &&
                          (CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE);

    let tooltipTitle = "";
    if (false === isSomeDataset) {
        tooltipTitle = "Some data must be ingested to enable search";
    } else if (isQueryStringEmpty) {
        tooltipTitle = "Enter query to search";
    }

    return (
        <Tooltip title={tooltipTitle}>
            <Button
                className={styles["gradientButton"] || ""}
                icon={<SearchOutlined/>}
                size={"large"}
                type={"primary"}
                disabled={isQueryStringEmpty || false === isSomeDataset ||
                          searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING}
                onClick={handleSubmitButtonClick}
            >
                Search
            </Button>
        </Tooltip>
    );
};

export default SubmitButton;
