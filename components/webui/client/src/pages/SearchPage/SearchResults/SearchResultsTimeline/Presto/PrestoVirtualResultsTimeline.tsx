import {
    useCallback,
    useEffect,
} from "react";

import {Dayjs} from "dayjs";

import ResultsTimeline from "../../../../../components/ResultsTimeline/index";
import {TimelineConfig} from "../../../../../components/ResultsTimeline/typings";
import {handlePrestoGuidedQuerySubmit} from "../../../SearchControls/Presto/presto-search-requests";
import {TIME_RANGE_OPTION} from "../../../SearchControls/TimeRangeInput/utils";
import useSearchStore from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {computeTimelineConfig} from "../utils";
import {useAggregationResults} from "./useAggregationResults";


/**
 * Renders timeline visualization of search results.
 *
 * @return
 */
const PrestoVirtualResultsTimeline = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const timelineConfig = useSearchStore((state) => state.timelineConfig);

    const aggregationResults = useAggregationResults();

    const handleTimelineZoom = useCallback((newTimeRange: [Dayjs, Dayjs]) => {
        const newTimelineConfig: TimelineConfig = computeTimelineConfig(newTimeRange);
        const {
            updateTimeRange,
            updateTimeRangeOption,
            updateTimelineConfig,
        } = useSearchStore.getState();

        // Update range picker selection to match zoomed range.
        updateTimeRange(newTimeRange);
        updateTimeRangeOption(TIME_RANGE_OPTION.CUSTOM);
        updateTimelineConfig(newTimelineConfig);

        handlePrestoGuidedQuerySubmit();
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

export default PrestoVirtualResultsTimeline;
