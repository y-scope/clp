import dayjs from "dayjs";
import DayjsDuration from "dayjs/plugin/duration";
import DayjsTimezone from "dayjs/plugin/timezone";
import DayjsUtc from "dayjs/plugin/utc";

import Duration, {DurationUnitType} from "dayjs/plugin/duration";


dayjs.extend(DayjsUtc);
dayjs.extend(DayjsDuration);
dayjs.extend(DayjsTimezone);

/**
 * The max number of data points in a timeline component.
 */
const MAX_DATA_POINTS_PER_TIMELINE = 40;

const DATETIME_FORMAT_TEMPLATE = "YYYY-MMM-DD HH:mm:ss";

/**
 * The time range and bucket durations of the ResultsTimeline component.
 */
interface TimelineConfig {
    range: {begin: dayjs.Dayjs; end: dayjs.Dayjs};
    bucketDuration: Duration.Duration;
}

/**
 * The type of TooltipItem.raw in chart.js.
 */
interface ChartTooltipItemRaw {
    x: number;
}


/**
 * An object that represents `count` log events at `timestamp`.
 */
interface TimelineBucket {
    timestamp: number;
    count: number;
}


type TimeRange = {
    begin: dayjs.Dayjs;
    end: dayjs.Dayjs;
};

enum TIME_UNIT {
    ALL = "all",
    MINUTE = "minute",
    HOUR = "hour",
    DAY = "day",
    WEEK = "week",
    MONTH = "month",
    YEAR = "year",
}

/**
 * Computes the timestamp range and bucket duration necessary to render the bars in the timeline
 * chart.
 *
 * @param timestampBeginUnixMillis
 * @param timestampEndUnixMillis
 * @return
 */
const computeTimelineConfig = (
    timestampBeginUnixMillis: number,
    timestampEndUnixMillis: number
): TimelineConfig => {
    const timeRangeMillis = timestampEndUnixMillis - timestampBeginUnixMillis;
    const exactTimelineBucketMillis = timeRangeMillis / MAX_DATA_POINTS_PER_TIMELINE;

    // A list of predefined bucket durations, ordered from least to greatest so that the
    // `durationSelections.find()` below can find the smallest bucket containing
    // `exactTimelineBucketMillis`.
    const durationSelections = [
        /* eslint-disable @stylistic/array-element-newline, no-magic-numbers */
        {unit: "second", values: [1, 2, 5, 10, 15, 30]},
        {unit: "minute", values: [1, 2, 5, 10, 15, 20, 30]},
        {unit: "hour", values: [1, 2, 3, 4, 8, 12]},
        {unit: "day", values: [1, 2, 5, 15]},
        {unit: "month", values: [1, 2, 3, 4, 6]},
        {unit: "year", values: [1]},
        /* eslint-enable @stylistic/array-element-newline, no-magic-numbers */
    ].flatMap(
        ({
            unit,
            values,
        }) => values.map(
            (value) => dayjs.duration(value, unit as DurationUnitType),
        ),
    );

    // bucketDuration is the smallest bucket containing `exactTimelineBucketMilis`.
    // This makes sure that the duration of timeline's time range is 1 sec, 2 sec, 5 sec and etc.
    const bucketDuration =
        durationSelections.find(
            (duration) => (exactTimelineBucketMillis <= duration.asMilliseconds()),
        ) ||
        dayjs.duration(
            Math.ceil(exactTimelineBucketMillis /
                dayjs.duration(1, TIME_UNIT.YEAR).asMilliseconds()),
            TIME_UNIT.YEAR,
        );

    return {
        range: expandTimeRangeToDurationMultiple(bucketDuration, {
            begin: dayjs.utc(timestampBeginUnixMillis),
            end: dayjs.utc(timestampEndUnixMillis),
        }),
        bucketDuration: bucketDuration,
    };
};

// eslint-disable-next-line no-warning-comments
// TODO: Switch date pickers so we don't have to do this hack
/**
 * Converts a UTC Dayjs object to a local-timezone JavaScript Date object that represents the same
 * date and time. In other words, the original year, month, day, hour, minute, second, and
 * millisecond appear unchanged.
 *
 * @param utcDatetime
 * @return The corresponding Date object
 */
const convertUtcDatetimeToSameLocalDate = (utcDatetime: dayjs.Dayjs) => {
    const localTz = dayjs.tz.guess();
    return utcDatetime.tz(localTz, true).toDate();
};

/**
 * Converts a local-timezone JavaScript Date object to a UTC Dayjs object that represents the same
 * date and time. In other words, the original year, month, day, hour, minute, second, and
 * millisecond appear unchanged.
 *
 * @param localDate
 * @return The corresponding Dayjs object
 */
const convertLocalDateToSameUtcDatetime = (localDate: Date) => {
    return dayjs(localDate).utc(true);
};

/**
 * Expands the time range so that both extremes are multiples of the given duration.
 *
 * @param duration
 * @param timeRange The time range to be expanded.
 * @param timeRange.begin
 * @param timeRange.end
 * @return The expanded time range.
 */
const expandTimeRangeToDurationMultiple = (duration: DayjsDuration.Duration, {
    begin,
    end,
}: TimeRange) => {
    const adjustedBegin = begin.valueOf() - (begin.valueOf() % duration.asMilliseconds());
    const adjustedEnd =
        Math.floor(
            (end.valueOf() + duration.asMilliseconds() - 1) / duration.asMilliseconds()
        ) * duration.asMilliseconds();

    return {begin: dayjs.utc(adjustedBegin), end: dayjs.utc(adjustedEnd)};
};

/**
 * Converts the timestamp from Chart.js' zoom plugin to a UTC Dayjs object.
 * NOTE: The Chart.js timescale operates in the local timezone, but we want to the timeline to
 * appear as if it's in UTC, so we apply the negative offset of the local timezone to all timestamps
 * before passing them to Chart.js. However, the zoom plugin thinks that Chart.js is displaying
 * timestamps in the local timezone, so it also applies the negative offset of the local timezone
 * before passing them to onZoom. So to get the original UTC timestamp, this method needs to apply
 * the local timezone offset twice.
 *
 * @param timestampUnixMillis
 * @return The corresponding Dayjs object
 */
const convertZoomTimestampToUtcDatetime = (timestampUnixMillis: number) => {
    // Create a Date object with given timestamp, which contains local timezone information.
    const initialDate = new Date(timestampUnixMillis);

    // Reverse local timezone offset.
    const intermediateDateTime = convertLocalDateToSameUtcDatetime(initialDate);

    // Reverse local timezone offset again.
    return convertLocalDateToSameUtcDatetime(intermediateDateTime.toDate());
};

export type {TimeRange};
export {
    convertLocalDateToSameUtcDatetime,
    convertUtcDatetimeToSameLocalDate,
    convertZoomTimestampToUtcDatetime,
    DATETIME_FORMAT_TEMPLATE,
    expandTimeRangeToDurationMultiple,
    TIME_UNIT,
    computeTimelineConfig
};
