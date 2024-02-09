import {DateTime} from "luxon";


const TIME_RANGE_UNIT = Object.freeze({
    ALL: "all",
    MINUTE: "minute",
    HOUR: "hour",
    DAY: "day",
    WEEK: "week",
    MONTH: "month",
    YEAR: "year",
});

const TIME_RANGE_MODIFIER = Object.freeze({
    NONE: "none",
    TODAY: "today",
    LAST: "last",
    PREV: "prev",
    TO_DATE: "to-date",
});

// TODO Switch date pickers so we don't have to do this hack
/**
 * Converts a DateTime object into a JavaScript Date object without changing the timestamp (hour,
 * minute, second, etc.).
 * @param dateTime
 * @returns {Date} The corresponding Date object
 */
const dateTimeToDateWithoutChangingTimestamp = (dateTime) => {
    return dateTime.toLocal().set({
        year: dateTime.year,
        month: dateTime.month,
        day: dateTime.day,
        hour: dateTime.hour,
        minute: dateTime.minute,
        second: dateTime.second,
        millisecond: dateTime.millisecond,
    }).toJSDate();
};

const TIME_RANGE_PRESET_LABEL = Object.freeze({
    [`${TIME_RANGE_UNIT.MINUTE}_${TIME_RANGE_MODIFIER.LAST}_15`]: "Last 15 Minutes",
    [`${TIME_RANGE_UNIT.MINUTE}_${TIME_RANGE_MODIFIER.LAST}_60`]: "Last 60 Minutes",
    [`${TIME_RANGE_UNIT.HOUR}_${TIME_RANGE_MODIFIER.LAST}_4`]: "Last 4 Hours",
    [`${TIME_RANGE_UNIT.HOUR}_${TIME_RANGE_MODIFIER.LAST}_24`]: "Last 24 Hours",
    [`${TIME_RANGE_UNIT.DAY}_${TIME_RANGE_MODIFIER.PREV}_1`]: "Previous Day",
    [`${TIME_RANGE_UNIT.WEEK}_${TIME_RANGE_MODIFIER.PREV}_1`]: "Previous Week",
    [`${TIME_RANGE_UNIT.MONTH}_${TIME_RANGE_MODIFIER.PREV}_1`]: "Previous Month",
    [`${TIME_RANGE_UNIT.YEAR}_${TIME_RANGE_MODIFIER.PREV}_1`]: "Previous Year",
    [`${TIME_RANGE_UNIT.DAY}_${TIME_RANGE_MODIFIER.TODAY}_0`]: "Today",
    [`${TIME_RANGE_UNIT.WEEK}_${TIME_RANGE_MODIFIER.TO_DATE}_0`]: "Week to Date",
    [`${TIME_RANGE_UNIT.MONTH}_${TIME_RANGE_MODIFIER.TO_DATE}_0`]: "Month to Date",
    [`${TIME_RANGE_UNIT.YEAR}_${TIME_RANGE_MODIFIER.TO_DATE}_0`]: "Year to Date",
    [`${TIME_RANGE_UNIT.ALL}_${TIME_RANGE_MODIFIER.NONE}_0`]: "All Time",
});

/**
 * Computes a time range based on a token.
 *
 * @param {string} token representing the time range to compute; format: `unit_modifier_amount`
 * @returns {Object} containing Date objects representing the computed begin and end time range
 */
const computeTimeRange = (token) => () => {
    const [unit, modifier, amount] = token.split("_");
    let endTime;
    let beginTime;

    if (TIME_RANGE_UNIT.ALL === unit) {
        endTime = DateTime.utc().plus({years: 1});
        beginTime = DateTime.fromMillis(0, {zone: "UTC"});
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

        endTime = (true === isEndingNow) ?
            DateTime.utc() :
            DateTime.utc().minus({[unit]: amount}).endOf(unit);
        beginTime = (true === isBeginStartOfUnit) ?
            endTime.startOf(unit) :
            endTime.minus({[unit]: amount});
    }

    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

/**
 * Changes the timezone of a given Date object to UTC without changing the time.
 *
 * @param {Date} date Date object to convert to UTC
 * @returns {Date} A new Date object with the same time values in UTC timezone
 */
const changeTimezoneToUtcWithoutChangingTime = (date) => {
    return new Date(Date.UTC(
        date.getFullYear(),
        date.getMonth(),
        date.getDate(),
        date.getHours(),
        date.getMinutes(),
        date.getSeconds(),
        date.getMilliseconds(),
    ));
};

const DEFAULT_TIME_RANGE = computeTimeRange(
    `${TIME_RANGE_UNIT.ALL}_${TIME_RANGE_MODIFIER.NONE}_0`,
);

export {
    TIME_RANGE_PRESET_LABEL,
    computeTimeRange,
    changeTimezoneToUtcWithoutChangingTime,
    DEFAULT_TIME_RANGE,
};
