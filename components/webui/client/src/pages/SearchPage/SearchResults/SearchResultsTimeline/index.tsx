import {
    useCallback,
    useEffect,
} from "react";

import {Card} from "antd";
import {Dayjs} from "dayjs";

import ResultsTimeline from "../../../../components/ResultsTimeline/index";
import {TimelineConfig} from "../../../../components/ResultsTimeline/typings";
import {
    CLP_QUERY_ENGINES,
    CLP_STORAGE_ENGINES,
    SETTINGS_QUERY_ENGINE,
    SETTINGS_STORAGE_ENGINE,
} from "../../../../config";
import {handlePrestoGuidedQuerySubmit} from "../../SearchControls/Presto/presto-search-requests";
import {handleQuerySubmit} from "../../SearchControls/search-requests";
import {TIME_RANGE_OPTION} from "../../SearchControls/TimeRangeInput/utils";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import {useAggregationResults} from "./useAggregationResults";
import {computeTimelineConfig} from "./utils";


/**
 * Renders timeline visualization of search results.
 *
 * @return
 */
const SearchResultsTimeline = () => {
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

        if (CLP_QUERY_ENGINES.PRESTO !== SETTINGS_QUERY_ENGINE) {
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
        } else {
            handlePrestoGuidedQuerySubmit();
        }
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
