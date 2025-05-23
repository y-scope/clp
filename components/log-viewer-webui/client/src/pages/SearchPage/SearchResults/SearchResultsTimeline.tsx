import {Card} from "antd";
import dayjs from "dayjs";

import ResultsTimeline, {
    computeTimelineConfig,
    TimelineBucket,
} from "../../../components/ResultsTimeline";
import {
    expandTimeRangeToDurationMultiple,
    TimeRange,
} from "../../../components/ResultsTimeline/datetime";
import {TIME_RANGE_OPTION} from "../SearchControls/TimeRangeInput/utils";
import useSearchStore from "../SearchState";
import {SearchResult} from "./SearchResultsTable/typings";


// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_RESULTS: SearchResult[] = [
    {
        id: 1,
        timestamp: "2023-01-01 12:00:00",
        message: "INFO: User login successful for user 'john.doe'.",
        filePath: "/var/logs/auth.log",
    },
    {
        id: 2,
        timestamp: "2023-01-01 12:01:00",
        message: "ERROR: Failed to connect to database 'logs_db'.",
        filePath: "/var/logs/db.log",
    },
    {
        id: 3,
        timestamp: "2023-01-01 12:02:00",
        message: "WARN: Disk space running low on volume '/var/logs'.",
        filePath: "/var/logs/system.log",
    },
    {
        id: 4,
        timestamp: "2023-01-01 12:03:00",
        message: "DEBUG: Processing request ID 12345.",
        filePath: "/var/logs/app.log",
    },
];

// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_BUCKETS: TimelineBucket[] = [
    {
        count: 2,
        timestamp: dayjs().subtract(1, "hour")
            .valueOf(),
    },
    {
        count: 3,
        // eslint-disable-next-line no-magic-numbers
        timestamp: dayjs().subtract(5, "hour")
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
    const {updateTimeRange, timeRange: [beginTime, endTime], setTimeRangeOption} = useSearchStore();
    const timestampBeginUnixMillis = beginTime.utc().valueOf();
    const timestampEndUnixMillis = endTime.utc().valueOf();

    if (0 === DUMMY_RESULTS.length) {
        return null;
    }

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
        setTimeRangeOption(TIME_RANGE_OPTION.CUSTOM);

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
