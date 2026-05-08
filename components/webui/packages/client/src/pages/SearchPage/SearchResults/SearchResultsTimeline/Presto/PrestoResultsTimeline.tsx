import {
    useCallback,
    useEffect,
} from "react";

import {Dayjs} from "dayjs";

import ResultsTimeline from "../../../../../components/ResultsTimeline/index";
import {TimelineConfig} from "../../../../../components/ResultsTimeline/typings";
import {
    buildPrestoGuidedQueries,
    handlePrestoGuidedQuerySubmit,
} from "../../../SearchControls/Presto/Guided/presto-guided-search-requests";
import {TIME_RANGE_OPTION} from "../../../SearchControls/TimeRangeInput/utils";
import useSearchStore from "../../../SearchState/index";
import usePrestoSearchState from "../../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {computeTimelineConfig} from "../utils";
import {usePrestoAggregationResults} from "./usePrestoAggregationResults";


/**
 * Renders timeline visualization of search results.
 *
 * @return
 */
const PrestoResultsTimeline = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const timelineConfig = useSearchStore((state) => state.timelineConfig);

    const aggregationResults = usePrestoAggregationResults();

    const handleTimelineZoom = useCallback((newTimeRange: [Dayjs, Dayjs]) => {
        const newTimelineConfig: TimelineConfig = computeTimelineConfig(newTimeRange);
        const {
            updateTimeRange,
            updateTimeRangeOption,
            updateTimelineConfig,
        } = useSearchStore.getState();

        const {updateCachedGuidedSearchQueryString} = usePrestoSearchState.getState();

        // Update range picker selection to match zoomed range.
        updateTimeRange(newTimeRange);
        updateTimeRangeOption(TIME_RANGE_OPTION.CUSTOM);
        updateTimelineConfig(newTimelineConfig);

        const {searchQueryString, timelineQueryString} = buildPrestoGuidedQueries(newTimeRange);
        updateCachedGuidedSearchQueryString(searchQueryString);
        handlePrestoGuidedQuerySubmit(searchQueryString, timelineQueryString);
    }, []);

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

export default PrestoResultsTimeline;
