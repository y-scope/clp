import {
    useCallback,
    useEffect,
} from "react";

import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {Dayjs} from "dayjs";

import ResultsTimeline from "../../../../../components/ResultsTimeline/index";
import {TimelineConfig} from "../../../../../components/ResultsTimeline/typings";
import {SETTINGS_STORAGE_ENGINE} from "../../../../../config";
import {handleQuerySubmit} from "../../../SearchControls/Native/search-requests";
import {TIME_RANGE_OPTION} from "../../../SearchControls/TimeRangeInput/utils";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {computeTimelineConfig} from "../utils";
import {useAggregationResults} from "./useAggregationResults";


/**
 * Renders timeline visualization of search results.
 *
 * @return
 */
const NativeResultsTimeline = () => {
    const queryIsCaseSensitive = useSearchStore((state) => state.queryIsCaseSensitive);
    const queryString = useSearchStore((state) => state.queryString);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const selectDataset = useSearchStore((state) => state.selectDataset);
    const timelineConfig = useSearchStore((state) => state.timelineConfig);

    const aggregationResults = useAggregationResults();

    const handleTimelineZoom = useCallback((newTimeRange: [Dayjs, Dayjs]) => {
        const newTimelineConfig: TimelineConfig = computeTimelineConfig(newTimeRange);
        const {
            updateTimeRange,
            updateTimeRangeOption,
            updateTimelineConfig,
            updateCachedDataset,
        } = useSearchStore.getState();

        // Update range picker selection to match zoomed range.
        updateTimeRange(newTimeRange);
        updateTimeRangeOption(TIME_RANGE_OPTION.CUSTOM);
        updateTimelineConfig(newTimelineConfig);

        const isQueryStringEmpty = queryString === SEARCH_STATE_DEFAULT.queryString;
        if (isQueryStringEmpty) {
            return;
        }

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
            timestampBegin: newTimeRange[0].valueOf(),
            timestampEnd: newTimeRange[1].valueOf(),
        });
    }, [
        queryIsCaseSensitive,
        queryString,
        selectDataset,
    ]);

    useEffect(() => {
        const numSearchResultsTimeline = aggregationResults?.reduce(
            (acc, curr) => acc + curr.count,
            0
        ) ?? 0;

        const {
            updateNumSearchResultsTimeline,
        } = useSearchStore.getState();

        updateNumSearchResultsTimeline(numSearchResultsTimeline);
    }, [
        aggregationResults,
    ]);

    return (
        <ResultsTimeline
            timelineConfig={timelineConfig}
            isInputDisabled={
                searchUiState === SEARCH_UI_STATE.QUERYING ||
                searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING
            }
            timelineBuckets={aggregationResults ?
                aggregationResults :
                []}
            onTimelineZoom={handleTimelineZoom}/>
    );
};

export default NativeResultsTimeline;
