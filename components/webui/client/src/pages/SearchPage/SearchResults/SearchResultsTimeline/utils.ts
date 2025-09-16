import dayjs from "dayjs";
import DayjsDuration, {DurationUnitType} from "dayjs/plugin/duration";
import DayjsUtc from "dayjs/plugin/utc";

import {TimeRange} from "../../../../components/ResultsTimeline/datetime/typings";
import {
    MAX_DATA_POINTS_PER_TIMELINE,
    TimelineConfig,
} from "../../../../components/ResultsTimeline/typings";


dayjs.extend(DayjsUtc);
dayjs.extend(DayjsDuration);

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
 * Computes the timestamp range and bucket duration necessary to render the bars in the timeline
 * chart.
 *
 * @param timeRange
 * @return
 */
const computeTimelineConfig = (
    timeRange: [dayjs.Dayjs, dayjs.Dayjs],
): TimelineConfig => {
    const timestampBeginUnixMillis = dayjs.utc(timeRange[0]).valueOf();
    const timestampEndUnixMillis = dayjs.utc(timeRange[1]).valueOf();
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

    const bucketDuration =
      durationSelections.find(
          (duration) => (exactTimelineBucketMillis <= duration.asMilliseconds()),
      ) ||

      // If the timeline bucket spans more than 1 year, calculate a custom multi-year duration.
      dayjs.duration(
          Math.ceil(exactTimelineBucketMillis /
              dayjs.duration(1, "year").asMilliseconds()),
          "year",
      );

    return {
        range: expandTimeRangeToDurationMultiple(bucketDuration, {
            begin: dayjs.utc(timestampBeginUnixMillis),
            end: dayjs.utc(timestampEndUnixMillis),
        }),
        bucketDuration: bucketDuration,
    };
};

export {
    computeTimelineConfig,
    expandTimeRangeToDurationMultiple,
};
