import {useEffect} from "react";

import {Card} from "antd";
import {Dayjs} from "dayjs";
import {TimelineConfig} from "src/components/ResultsTimeline/typings";

import ResultsTimeline from "../../../../components/ResultsTimeline/index";
import {handleQuerySubmit} from "../../SearchControls/search-requests";
import {TIME_RANGE_OPTION} from "../../SearchControls/TimeRangeInput/utils";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import {useAggregationResults} from "./useAggregationResults";
import {computeTimelineConfig} from "./utils";
import Dataset from "../../SearchControls/Dataset";


/**
 * Renders timeline visualization of search results.
 *
 * @return
 */
const SearchResultsTimeline = () => {
    const queryString = useSearchStore((state) => state.queryString);
    const updateTimeRange = useSearchStore((state) => state.updateTimeRange);
    const updateTimeRangeOption = useSearchStore((state) => state.updateTimeRangeOption);
    const timelineConfig = useSearchStore((state) => state.timelineConfig);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const updateTimelineConfig = useSearchStore((state) => state.updateTimelineConfig);
    const updateNumSearchResultsTimeline = useSearchStore((state) => state.updateNumSearchResultsTimeline);
    const selectDataset = useSearchStore((state) => state.selectDataset);
    const updateCachedDataset = useSearchStore((state) => state.updateCachedDataset);

    const aggregationResults = useAggregationResults();

    useEffect(() => {
        const numSearchResultsTimeline = aggregationResults?.reduce(
            (acc, curr) => acc + curr.count,
            0
        ) ?? 0;

        updateNumSearchResultsTimeline(numSearchResultsTimeline);
    }, [
        aggregationResults,
        updateNumSearchResultsTimeline,
    ]);

    const handleTimelineZoom = (newTimeRange: [Dayjs, Dayjs]) => {
        const newTimelineConfig: TimelineConfig = computeTimelineConfig(newTimeRange);

        // Update range picker selection to match zoomed range.
        updateTimeRange(newTimeRange);
        updateTimeRangeOption(TIME_RANGE_OPTION.CUSTOM);
        updateTimelineConfig(newTimelineConfig);

        const isQueryStringEmpty = queryString === SEARCH_STATE_DEFAULT.queryString;
        if (isQueryStringEmpty) {
            return;
        }

        let storageEngine = "clp-s";
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
            timestampBegin: newTimeRange[0].valueOf(),
            timestampEnd: newTimeRange[1].valueOf(),
        });
    };

    return (
        <Card>
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
        </Card>
    );
};

export default SearchResultsTimeline;
