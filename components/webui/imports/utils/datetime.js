import dayjs from "dayjs";
import Duration from "dayjs/plugin/duration";
import Timezone from "dayjs/plugin/timezone";
import Utc from "dayjs/plugin/utc";


dayjs.extend(Utc);
dayjs.extend(Timezone);
dayjs.extend(Duration);

const DATETIME_FORMAT_TEMPLATE = "YYYY-MMM-DD HH:mm:ss";

/* eslint-disable sort-keys */
/**
 * Enum of time units.
 *
 * @enum {string}
 */
const TIME_UNIT = Object.freeze({
    ALL: "all",
    MINUTE: "minute",
    HOUR: "hour",
    DAY: "day",
    WEEK: "week",
    MONTH: "month",
    YEAR: "year",
});
/* eslint-enable sort-keys */

/* eslint-disable sort-keys */
/**
 * Enum of time range modifiers.
 *
 * @enum {string}
 */
const TIME_RANGE_MODIFIER = Object.freeze({
    NONE: "none",
    TODAY: "today",
    LAST: "last",
    PREV: "prev",
    TO_DATE: "to-date",
});
/* eslint-enable sort-keys */

/**
 * Time range presets.
 *
 * @type {{[key: string]: string}}
 */
const TIME_RANGE_PRESET_LABEL = Object.freeze({
    [`${TIME_UNIT.MINUTE}_${TIME_RANGE_MODIFIER.LAST}_15`]: "Last 15 Minutes",
    [`${TIME_UNIT.MINUTE}_${TIME_RANGE_MODIFIER.LAST}_60`]: "Last 60 Minutes",
    [`${TIME_UNIT.HOUR}_${TIME_RANGE_MODIFIER.LAST}_4`]: "Last 4 Hours",
    [`${TIME_UNIT.HOUR}_${TIME_RANGE_MODIFIER.LAST}_24`]: "Last 24 Hours",
    [`${TIME_UNIT.DAY}_${TIME_RANGE_MODIFIER.PREV}_1`]: "Previous Day",
    [`${TIME_UNIT.WEEK}_${TIME_RANGE_MODIFIER.PREV}_1`]: "Previous Week",
    [`${TIME_UNIT.MONTH}_${TIME_RANGE_MODIFIER.PREV}_1`]: "Previous Month",
    [`${TIME_UNIT.YEAR}_${TIME_RANGE_MODIFIER.PREV}_1`]: "Previous Year",
    [`${TIME_UNIT.DAY}_${TIME_RANGE_MODIFIER.TODAY}_0`]: "Today",
    [`${TIME_UNIT.WEEK}_${TIME_RANGE_MODIFIER.TO_DATE}_0`]: "Week to Date",
    [`${TIME_UNIT.MONTH}_${TIME_RANGE_MODIFIER.TO_DATE}_0`]: "Month to Date",
    [`${TIME_UNIT.YEAR}_${TIME_RANGE_MODIFIER.TO_DATE}_0`]: "Year to Date",
    [`${TIME_UNIT.ALL}_${TIME_RANGE_MODIFIER.NONE}_0`]: "All Time",
});

/**
 * Computes a time range based on a token.
 *
 * @param {string} token representing the time range to compute; format: `unit_modifier_amount`
 * @return {TimeRange} The computed time range
 */
const computeTimeRange = (token) => {
    const [
        unit,
        modifier,
        amount,
    ] = token.split("_");
    let end;
    let begin;

    if (TIME_UNIT.ALL === unit) {
        end = dayjs.utc().add(1, TIME_UNIT.YEAR);
        begin = dayjs.utc(0);
    } else {
        const isEndingNow = [
            TIME_RANGE_MODIFIER.LAST,
            TIME_RANGE_MODIFIER.TODAY,
            TIME_RANGE_MODIFIER.TO_DATE,
        ].includes(modifier);
        const isBeginStartOfUnit = [
            TIME_RANGE_MODIFIER.PREV,
            TIME_RANGE_MODIFIER.TODAY,
            TIME_RANGE_MODIFIER.TO_DATE,
        ].includes(modifier);

        end = (true === isEndingNow) ?
            dayjs.utc() :
            dayjs.utc()
                .subtract(amount, unit)
                .endOf(unit);
        begin = (true === isBeginStartOfUnit) ?
            end.startOf(unit) :
            end.subtract(amount, unit);
    }

    return {
        begin,
        end,
    };
};

const DEFAULT_TIME_RANGE = computeTimeRange(
    `${TIME_UNIT.ALL}_${TIME_RANGE_MODIFIER.NONE}_0`,
);

// eslint-disable-next-line no-warning-comments
// TODO: Switch date pickers so we don't have to do this hack
/**
 * Converts a UTC Dayjs object to a local-timezone JavaScript Date object that represents the same
 * date and time. In other words, the original year, month, day, hour, minute, second, and
 * millisecond appear unchanged.
 *
 * @param {dayjs.Dayjs} utcDatetime
 * @return {Date} The corresponding Date object
 */
const convertUtcDatetimeToSameLocalDate = (utcDatetime) => {
    const localTz = dayjs.tz.guess();
    return utcDatetime.tz(localTz, true).toDate();
};

/**
 * Converts a local-timezone JavaScript Date object to a UTC Dayjs object that represents the same
 * date and time. In other words, the original year, month, day, hour, minute, second, and
 * millisecond appear unchanged.
 *
 * @param {Date} localDate
 * @return {dayjs.Dayjs} The corresponding Dayjs object
 */
const convertLocalDateToSameUtcDatetime = (localDate) => {
    return dayjs(localDate).utc(true);
};

/**
 * Expands the time range so that both extremes are multiples of the given duration.
 *
 * @param {dayjs.Duration} duration
 * @param {TimeRange} timeRange The time range to be expanded.
 * @return {TimeRange} The expanded time range.
 */
const expandTimeRangeToDurationMultiple = (duration, {
    begin,
    end,
}) => {
    const adjustedBegin = begin - (begin % duration.asMilliseconds());
    const adjustedEnd =
        Math.floor(
            (end + duration.asMilliseconds() - 1) / duration.asMilliseconds()
        ) * duration.asMilliseconds();

    return {begin: dayjs.utc(adjustedBegin), end: dayjs.utc(adjustedEnd)};
};

export {
    computeTimeRange,
    convertLocalDateToSameUtcDatetime,
    convertUtcDatetimeToSameLocalDate,
    DATETIME_FORMAT_TEMPLATE,
    DEFAULT_TIME_RANGE,
    expandTimeRangeToDurationMultiple,
    TIME_RANGE_PRESET_LABEL,
    TIME_UNIT,
};
