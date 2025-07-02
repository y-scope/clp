import dayjs from "dayjs";
import utc from "dayjs/plugin/utc";


dayjs.extend(utc);

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
    LAST_12_MONTHS = "Last 12 Months",
    MONTH_TO_DATE = "Month to Date",
    YEAR_TO_DATE = "Year to Date",
    ALL_TIME = "All Time",
    CUSTOM = "Custom",
}

const DEFAULT_TIME_RANGE = TIME_RANGE_OPTION.ALL_TIME;

/* eslint-disable no-magic-numbers */
const TIME_RANGE_OPTION_DAYJS_MAP: Record<TIME_RANGE_OPTION, () => [dayjs.Dayjs, dayjs.Dayjs]> = {
    [TIME_RANGE_OPTION.LAST_15_MINUTES]: () => [
        dayjs().utc()
            .subtract(15, "minute"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.LAST_HOUR]: () => [
        dayjs().utc()
            .subtract(1, "hour"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.TODAY]: () => [
        dayjs().utc()
            .startOf("day"),
        dayjs().utc()
            .endOf("day"),
    ],
    [TIME_RANGE_OPTION.YESTERDAY]: () => [
        dayjs().utc()
            .subtract(1, "day")
            .startOf("day"),
        dayjs().utc()
            .subtract(1, "day")
            .endOf("day"),
    ],
    [TIME_RANGE_OPTION.LAST_7_DAYS]: () => [
        dayjs().utc()
            .subtract(7, "day"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.LAST_30_DAYS]: () => [
        dayjs().utc()
            .subtract(30, "day"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.LAST_12_MONTHS]: () => [
        dayjs().utc()
            .subtract(12, "month"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.MONTH_TO_DATE]: () => [
        dayjs().utc()
            .startOf("month"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.YEAR_TO_DATE]: () => [
        dayjs().utc()
            .startOf("year"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.ALL_TIME]: () => [
        dayjs(0).utc(),
        dayjs().utc()
            .add(1, "year"),
    ],

    // Custom option is just a placeholder for typing purposes, its DayJs values should not
    // be used.
    [TIME_RANGE_OPTION.CUSTOM]: () => [
        dayjs().utc(),
        dayjs().utc(),
    ],
};


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
    dates: [dayjs.Dayjs | null, dayjs.Dayjs | null] | null
): dates is [dayjs.Dayjs, dayjs.Dayjs] => {
    return null !== dates && null !== dates[0] && null !== dates[1];
};


export {
    DEFAULT_TIME_RANGE,
    isValidDateRange,
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
    TIME_RANGE_OPTION_NAMES,
};
