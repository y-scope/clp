import dayjs from "dayjs";

import ResultsTimeline, {
    computeTimelineConfig,
    TimelineBucket,
} from "../../../components/ResultsTimeline";
import {
    expandTimeRangeToDurationMultiple,
    TimeRange,
} from "../../../utils/datetime";
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
const DUMMUY_BUCKETS: TimelineBucket[] = [
    {
        count: 2,
        timestamp: dayjs().subtract(1, "hour")
            .valueOf(),
    },
    {
        count: 3,
        timestamp: dayjs().subtract(1, "minutes")
            .valueOf(),
    },
    {
        count: 5,
        timestamp: dayjs().valueOf(),
    },
];

/**
 * Renders search results in a table.
 *
 * @return
 */
const SearchResultsTimeline = () => {
    if (0 === DUMMY_RESULTS.length) {
        return null;
    }

    const timestamps = DUMMUY_BUCKETS.map(({timestamp}) => timestamp);
    const begin = Math.min(...timestamps);
    const end = Math.max(...timestamps);

    const timelineConfig = computeTimelineConfig(begin, end);

    const handleTimelineZoom = (newTimeRange: TimeRange) => {
        // Expand the time range to the granularity of buckets so if the user
        // pans across at least one bar in the graph, we will zoom into a region
        // that still contains log events.
        const expandedTimeRange = expandTimeRangeToDurationMultiple(
            timelineConfig.bucketDuration,
            newTimeRange,
        );

        console.log(expandedTimeRange);

        // handleQuerySubmit(expandedTimeRange);
    };

    return (
        <ResultsTimeline
            isInputDisabled={false}
            timelineBuckets={DUMMUY_BUCKETS}
            timelineConfig={timelineConfig}
            onTimelineZoom={handleTimelineZoom}/>
    );
};

export default SearchResultsTimeline;
