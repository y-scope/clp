import {
    useCallback,
    useEffect,
} from "react";

import {Dayjs} from "dayjs";

import ResultsTimeline from "../../../../../components/ResultsTimeline/index";
import {TimelineConfig} from "../../../../../components/ResultsTimeline/typings";
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
    const selectedDatasets = useSearchStore((state) => state.selectedDatasets);
    const timelineConfig = useSearchStore((state) => state.timelineConfig);
    const maxNumResults = useSearchStore((state) => state.maxNumResults);

    const aggregationResults = useAggregationResults();

    const handleTimelineZoom = useCallback((newTimeRange: [Dayjs, Dayjs]) => {
        const newTimelineConfig: TimelineConfig = computeTimelineConfig(newTimeRange);
        const {
            updateTimeRange,
            updateTimeRangeOption,
            updateTimelineConfig,
            updateQueriedDatasets,
        } = useSearchStore.getState();

        // Update range picker selection to match zoomed range.
        updateTimeRange(newTimeRange);
        updateTimeRangeOption(TIME_RANGE_OPTION.CUSTOM);
        updateTimelineConfig(newTimelineConfig);

        const isQueryStringEmpty = queryString === SEARCH_STATE_DEFAULT.queryString;
        if (isQueryStringEmpty) {
            return;
        }

        if (0 === selectedDatasets.length) {
            console.error("Cannot submit a query without a dataset selection.");

            return;
        }
        updateQueriedDatasets(selectedDatasets);

        handleQuerySubmit({
            datasets: selectedDatasets,
            ignoreCase: false === queryIsCaseSensitive,
            maxNumResults: maxNumResults,
            queryString: queryString,
            timeRangeBucketSizeMillis: newTimelineConfig.bucketDuration.asMilliseconds(),
            timestampBegin: newTimeRange[0].valueOf(),
            timestampEnd: newTimeRange[1].valueOf(),
        });
    }, [
        maxNumResults,
        queryIsCaseSensitive,
        queryString,
        selectedDatasets,
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
