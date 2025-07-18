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
import useSearchStore from "../../../SearchState/index";
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
    const queryIsCaseSensitive = useSearchStore((state) => state.queryIsCaseSensitive);
    const queryString = useSearchStore((state) => state.queryString);
    const selectDataset = useSearchStore((state) => state.selectDataset);
    const updateCachedDataset = useSearchStore((state) => state.updateCachedDataset);

    /**
     * Submits search query.
     */
    const handleSubmitButtonClick = useCallback(() => {
        // Update timeline to match range picker selection.
        const newTimelineConfig = computeTimelineConfig(timeRange);
        const {updateTimelineConfig} = useSearchStore.getState();
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
            dataset: selectDataset,
            ignoreCase: false === queryIsCaseSensitive,
            queryString: queryString,
            timeRangeBucketSizeMillis: newTimelineConfig.bucketDuration.asMilliseconds(),
            timestampBegin: timeRange[0].valueOf(),
            timestampEnd: timeRange[1].valueOf(),
        });
    }, [queryString,
        queryIsCaseSensitive,
        timeRange,
        selectDataset,
        updateCachedDataset]);

    const isQueryStringEmpty = "" === queryString;

    // Submit button must be disabled if there are no datasets since clp-s requires dataset option
    // for queries.
    const isNoDatasetsAndClpS = (null === selectDataset) &&
                          (CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE);

    let tooltipTitle = "";
    if (isNoDatasetsAndClpS) {
        tooltipTitle = "Some data must be ingested to enable search";
    } else if (isQueryStringEmpty) {
        tooltipTitle = "Enter query to search";
    }

    return (
        <Tooltip title={tooltipTitle}>
            <Button
                className={styles["gradientButton"] || ""}
                htmlType={"submit"}
                icon={<SearchOutlined/>}
                size={"large"}
                type={"primary"}
                disabled={isQueryStringEmpty || isNoDatasetsAndClpS ||
                          searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING}
                onClick={handleSubmitButtonClick}
            >
                Search
            </Button>
        </Tooltip>
    );
};

export default SubmitButton;
