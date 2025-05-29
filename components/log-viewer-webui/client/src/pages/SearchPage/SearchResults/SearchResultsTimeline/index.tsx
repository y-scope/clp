import {Card} from "antd";

import ResultsTimeline from "../../../../components/ResultsTimeline/index";
import {TIME_RANGE_OPTION} from "../../SearchControls/TimeRangeInput/utils";
import useSearchStore from "../../SearchState/index";
import {
    computeTimelineConfig,
} from "./utils";
import { SEARCH_UI_STATE } from "../../SearchState/typings";
import { useAggregationResults } from "./useAggregationResults";
import { handleQuerySubmit } from "../../SearchControls/search-requests";
import {SEARCH_STATE_DEFAULT} from "../../SearchState/index";
import { TimelineConfig } from "src/components/ResultsTimeline/typings";
import { Dayjs } from "dayjs";

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

        handleQuerySubmit({
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
