import dayjs, {Dayjs} from "dayjs";
import utc from "dayjs/plugin/utc";

import useSearchStore from "../../SearchState";
import {fetchAllTimeRange} from "./sql";


dayjs.extend(utc);


const DEFAULT_TIME_RANGE: [Dayjs, Dayjs] = [
    dayjs(0).utc(),
    dayjs().utc()
        .add(1, "year"),
];


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

const DEFAULT_TIME_RANGE_OPTION = TIME_RANGE_OPTION.ALL_TIME;

/* eslint-disable no-magic-numbers, @typescript-eslint/require-await */
const TIME_RANGE_OPTION_DAYJS_MAP: Record<
    TIME_RANGE_OPTION,
    () => Promise<[dayjs.Dayjs, dayjs.Dayjs]>
> = {
    [TIME_RANGE_OPTION.LAST_15_MINUTES]: async () => [
        dayjs().utc()
            .subtract(15, "minute"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.LAST_HOUR]: async () => [
        dayjs().utc()
            .subtract(1, "hour"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.TODAY]: async () => [
        dayjs().utc()
            .startOf("day"),
        dayjs().utc()
            .endOf("day"),
    ],
    [TIME_RANGE_OPTION.YESTERDAY]: async () => [
        dayjs().utc()
            .subtract(1, "day")
            .startOf("day"),
        dayjs().utc()
            .subtract(1, "day")
            .endOf("day"),
    ],
    [TIME_RANGE_OPTION.LAST_7_DAYS]: async () => [
        dayjs().utc()
            .subtract(7, "day"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.LAST_30_DAYS]: async () => [
        dayjs().utc()
            .subtract(30, "day"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.LAST_12_MONTHS]: async () => [
        dayjs().utc()
            .subtract(12, "month"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.MONTH_TO_DATE]: async () => [
        dayjs().utc()
            .startOf("month"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.YEAR_TO_DATE]: async () => [
        dayjs().utc()
            .startOf("year"),
        dayjs().utc(),
    ],
    [TIME_RANGE_OPTION.ALL_TIME]: async () => {
        const {selectedDatasets} = useSearchStore.getState();
        return fetchAllTimeRange(selectedDatasets);
    },

    // Custom option is just a placeholder for typing purposes, its DayJs values should not
    // be used.
    [TIME_RANGE_OPTION.CUSTOM]: async () => [
        dayjs().utc(),
        dayjs().utc(),
    ],
};
/* eslint-enable no-magic-numbers, @typescript-eslint/require-await */


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

/**
 * AntD date range position.
 */
enum DATE_RANGE_POSITION {
    START = "start",
    END = "end",
}

/**
 * AntD RangePicker prop key for date range position.
 */
const DATE_RANGE_PROP_KEY = "date-range";

/**
 * Map of time range options to their display text
 */
const TIME_RANGE_DISPLAY_TEXT_MAP: Record<
    TIME_RANGE_OPTION,
    Record<DATE_RANGE_POSITION, string>
> = {
    [TIME_RANGE_OPTION.LAST_15_MINUTES]: {
        [DATE_RANGE_POSITION.START]: "15 minutes ago",
        [DATE_RANGE_POSITION.END]: "Now",
    },
    [TIME_RANGE_OPTION.LAST_HOUR]: {
        [DATE_RANGE_POSITION.START]: "1 hour ago",
        [DATE_RANGE_POSITION.END]: "Now",
    },
    [TIME_RANGE_OPTION.TODAY]: {
        [DATE_RANGE_POSITION.START]: "Start of today",
        [DATE_RANGE_POSITION.END]: "End of today",
    },
    [TIME_RANGE_OPTION.YESTERDAY]: {
        [DATE_RANGE_POSITION.START]: "Start of yesterday",
        [DATE_RANGE_POSITION.END]: "End of yesterday",
    },
    [TIME_RANGE_OPTION.LAST_7_DAYS]: {
        [DATE_RANGE_POSITION.START]: "7 days ago",
        [DATE_RANGE_POSITION.END]: "Now",
    },
    [TIME_RANGE_OPTION.LAST_30_DAYS]: {
        [DATE_RANGE_POSITION.START]: "30 days ago",
        [DATE_RANGE_POSITION.END]: "Now",
    },
    [TIME_RANGE_OPTION.LAST_12_MONTHS]: {
        [DATE_RANGE_POSITION.START]: "12 months ago",
        [DATE_RANGE_POSITION.END]: "Now",
    },
    [TIME_RANGE_OPTION.MONTH_TO_DATE]: {
        [DATE_RANGE_POSITION.START]: "Start of month",
        [DATE_RANGE_POSITION.END]: "Now",
    },
    [TIME_RANGE_OPTION.YEAR_TO_DATE]: {
        [DATE_RANGE_POSITION.START]: "Start of year",
        [DATE_RANGE_POSITION.END]: "Now",
    },
    [TIME_RANGE_OPTION.ALL_TIME]: {
        [DATE_RANGE_POSITION.START]: "First timestamp",
        [DATE_RANGE_POSITION.END]: "Last timestamp",
    },

    // Custom option is just a placeholder for typing purposes, its values should not
    // be used.
    [TIME_RANGE_OPTION.CUSTOM]: {
        [DATE_RANGE_POSITION.START]: "Start date",
        [DATE_RANGE_POSITION.END]: "End date",
    },
};


export {
    DATE_RANGE_POSITION,
    DATE_RANGE_PROP_KEY,
    DEFAULT_TIME_RANGE,
    DEFAULT_TIME_RANGE_OPTION,
    isValidDateRange,
    TIME_RANGE_DISPLAY_TEXT_MAP,
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
    TIME_RANGE_OPTION_NAMES,
};
