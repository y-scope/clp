import {Card} from "antd";

import {TimeRange} from "../../../../components/ResultsTimeline/datetime/typings";
import ResultsTimeline from "../../../../components/ResultsTimeline/index";
import {TIME_RANGE_OPTION} from "../../SearchControls/TimeRangeInput/utils";
import useSearchStore from "../../SearchState/index";
import {
    computeTimelineConfig,
    expandTimeRangeToDurationMultiple,
} from "./utils";
import { SEARCH_UI_STATE } from "../../SearchState/typings";
import { useAggregationResults } from "../../reactive-mongo-queries/useAggregationResults";
import { handleQuerySubmit } from "../../SearchControls/search-requests";
import {SEARCH_STATE_DEFAULT} from "../../SearchState/index";
import { TimelineConfig } from "src/components/ResultsTimeline/typings";

/**
 * Renders timeline visualization of search results.
 *
 * @return
 */
const SearchResultsTimeline = () => {
    const {
        queryString,
        updateTimeRange,
        updateTimeRangeOption,
        timelineConfig,
        searchUiState,
        updateTimelineConfig,
    } = useSearchStore();

    const aggregationResults = useAggregationResults();
    console.log("aggregationResults", aggregationResults);

    const handleTimelineZoom = (newTimeRange: TimeRange) => {
        console.log("handleTimelineZoom", newTimeRange);

        const newTimelineConfig: TimelineConfig = computeTimelineConfig(
            [newTimeRange.begin,
            newTimeRange.end]
        );



        updateTimeRange([newTimeRange.begin,
            newTimeRange.end]);
        updateTimeRangeOption(TIME_RANGE_OPTION.CUSTOM);
        updateTimelineConfig(newTimelineConfig);

        console.log("this is duration", newTimelineConfig.bucketDuration.asMilliseconds());

        const isQueryStringEmpty = queryString === SEARCH_STATE_DEFAULT.queryString;

        if (isQueryStringEmpty) {
            // If the query string is empty, we do not submit a new query. However, we can still
            // update the timeline config and range picker.
            return;
        }

        handleQuerySubmit({
            ignoreCase: false,
            queryString: queryString,
            timeRangeBucketSizeMillis: newTimelineConfig.bucketDuration.asMilliseconds(),
            timestampBegin: newTimeRange.begin.valueOf(),
            timestampEnd: newTimeRange.end.valueOf(),
        });

    };

    return (
        <Card>
            <ResultsTimeline
                isInputDisabled={
                    searchUiState === SEARCH_UI_STATE.QUERYING ||
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING
                }
                timelineBuckets={aggregationResults ?
                    aggregationResults :
                    []}
                timelineConfig={timelineConfig}
                onTimelineZoom={handleTimelineZoom}/>
        </Card>
    );
};

export default SearchResultsTimeline;
