import dayjs from "dayjs";

import {TimeRange} from "../../typings/time";


/**
 * Time range options.
 */
enum TIME_RANGE_OPTION {
    LAST_15_MINUTES = "Last 15 Minutes",
    LAST_HOUR = "Last Hour",
    TODAY = "Today",
    YESTERDAY = "Yesterday",
    LAST_7_DAYS = "Last 7 Days",
    LAST_30_DAYS = "Last 30 Days",
    MONTH_TO_DATE = "Month to Date",
    CUSTOM = "Custom",
}

const DEFAULT_TIME_RANGE = TIME_RANGE_OPTION.TODAY;

/* eslint-disable no-magic-numbers */
/**
 * Maps a predefined set of time range options to DayJs date ranges.
 */
const TIME_RANGE_OPTION_DAYJS_MAP: Readonly<Record<TIME_RANGE_OPTION, TimeRange>> = Object.freeze({
    [TIME_RANGE_OPTION.LAST_15_MINUTES]: [
        dayjs().subtract(15, "minute"),
        dayjs(),
    ],
    [TIME_RANGE_OPTION.LAST_HOUR]: [
        dayjs().subtract(1, "hour"),
        dayjs(),
    ],
    [TIME_RANGE_OPTION.TODAY]: [
        dayjs().startOf("day"),
        dayjs().endOf("day"),
    ],
    [TIME_RANGE_OPTION.YESTERDAY]: [
        dayjs().subtract(1, "d"),
        dayjs().subtract(1, "d"),
    ],
    [TIME_RANGE_OPTION.LAST_7_DAYS]: [
        dayjs().subtract(7, "d"),
        dayjs(),
    ],
    [TIME_RANGE_OPTION.LAST_30_DAYS]: [
        dayjs().subtract(30, "d"),
        dayjs(),
    ],
    [TIME_RANGE_OPTION.MONTH_TO_DATE]: [
        dayjs().startOf("month"),
        dayjs(),
    ],

    // Custom option is just a placeholder for typing purposes, its DayJs values should not
    // be used.
    [TIME_RANGE_OPTION.CUSTOM]: [
        dayjs(),
        dayjs(),
    ],
});


/**
 * Key names in enum `TIME_RANGE_OPTION`.
 */
const TIME_RANGE_OPTION_NAMES = Object.freeze(
    Object.values(TIME_RANGE_OPTION).filter((value) => "string" === typeof value)
);

/**
 * Validates dates provided by the range picker callback are non-null.
 *
 * @param dates
 * @return
 */
const isValidDateRange = (
    dates: TimeRange | null
): dates is TimeRange => {
    return null !== dates && null !== dates[0] && null !== dates[1];
};


export {
    DEFAULT_TIME_RANGE,
    isValidDateRange,
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
    TIME_RANGE_OPTION_NAMES,
};
