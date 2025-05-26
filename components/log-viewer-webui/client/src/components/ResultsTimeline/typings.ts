import dayjs from "dayjs";
import Duration from "dayjs/plugin/duration";


/**
 * The time range and bucket durations of the ResultsTimeline component.
 */
interface TimelineConfig {
    range: {begin: dayjs.Dayjs; end: dayjs.Dayjs};
    bucketDuration: Duration.Duration;
}


/**
 * An object that represents `count` log events at `timestamp`.
 */
interface TimelineBucket {
    timestamp: number;
    count: number;
}

/**
 * The max number of data points in a timeline component.
 */
const MAX_DATA_POINTS_PER_TIMELINE = 40;

export type {
    TimelineBucket,
    TimelineConfig,
};
export {MAX_DATA_POINTS_PER_TIMELINE};
