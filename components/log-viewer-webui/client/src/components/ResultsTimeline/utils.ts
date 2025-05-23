import dayjs from "dayjs";
import {DurationUnitType} from "dayjs/plugin/duration";

import {
    expandTimeRangeToDurationMultiple,
    TIME_UNIT,
} from "../../utils/datetime";
import {MAX_DATA_POINTS_PER_TIMELINE} from "./TimelineBucket";
import {TimelineConfig} from "./typings";


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


/**
 * Deselects all selections within the browser viewport.
 */
const deselectAll = () => {
    const selection = window.getSelection();
    if (null !== selection) {
        selection.removeAllRanges();
    }
};

export {
    computeTimelineConfig, deselectAll,
};
