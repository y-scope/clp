import dayjs from "dayjs";
import Duration from "dayjs/plugin/duration";
import Timezone from "dayjs/plugin/timezone";
import Utc from "dayjs/plugin/utc";

import dayjs from "dayjs";
import {DurationUnitType} from "dayjs/plugin/duration";

import {
    expandTimeRangeToDurationMultiple,
    TIME_UNIT,
} from "./datetime";
import {
    MAX_DATA_POINTS_PER_TIMELINE,
    TimelineBucket,
    TimelineConfig,
} from "./typings";



dayjs.extend(Utc);
dayjs.extend(Timezone);
dayjs.extend(Duration);

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

