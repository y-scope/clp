import {Card} from "antd";
import dayjs from "dayjs";

import {TimeRange} from "../../../../components/ResultsTimeline/datetime/typings";
import ResultsTimeline from "../../../../components/ResultsTimeline/index";
import {TimelineBucket} from "../../../../components/ResultsTimeline/typings";
import {TIME_RANGE_OPTION} from "../../SearchControls/TimeRangeInput/utils";
import useSearchStore from "../../SearchState";
import {
    computeTimelineConfig,
    expandTimeRangeToDurationMultiple,
} from "./utils";


// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_BUCKETS: TimelineBucket[] = [
    {
        count: 2,
        timestamp: dayjs().subtract(10, "day")
            .valueOf(),
    },
    {
        count: 3,
        // eslint-disable-next-line no-magic-numbers
        timestamp: dayjs().subtract(5, "day")
            .valueOf(),
    },
    {
        count: 5,
        timestamp: dayjs().valueOf(),
    },
];

/**
 * Renders timeline visualization of search results.
 *
 * @return
 */
const SearchResultsTimeline = () => {
    const {
        updateTimeRange,
        timeRange: [beginTime, endTime],
        updateTimeRangeOption,
    } = useSearchStore();
    const timestampBeginUnixMillis = beginTime.utc().valueOf();
    const timestampEndUnixMillis = endTime.utc().valueOf();

    const timelineConfig = computeTimelineConfig(timestampBeginUnixMillis, timestampEndUnixMillis);

    const handleTimelineZoom = (newTimeRange: TimeRange) => {
        // Expand the time range to the granularity of buckets so if the user
        // pans across at least one bar in the graph, we will zoom into a region
        // that still contains log events.
        const expandedTimeRange = expandTimeRangeToDurationMultiple(
            timelineConfig.bucketDuration,
            newTimeRange,
        );

        updateTimeRange([expandedTimeRange.begin,
            expandedTimeRange.end]);
        updateTimeRangeOption(TIME_RANGE_OPTION.CUSTOM);

        // eslint-disable-next-line no-warning-comments
        // TODO: submit query based on timelineConfig.
    };

    // eslint-disable-next-line no-warning-comments
    // TODO: fix `isInputDisabled` .
    return (
        <Card>
            <ResultsTimeline
                isInputDisabled={false}
                timelineBuckets={DUMMY_BUCKETS}
                timelineConfig={timelineConfig}
                onTimelineZoom={handleTimelineZoom}/>
        </Card>
    );
};

export default SearchResultsTimeline;
