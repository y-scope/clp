import {Nullable} from "@webui/common/utility-types";
import dayjs, {Dayjs} from "dayjs";
import utc from "dayjs/plugin/utc";

import {querySql} from "../../../../api/sql";
import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../../config";
import {
    buildClpsTimeRangeSql,
    buildClpTimeRangeSql,
} from "./timeRangeSql";


dayjs.extend(utc);


const TIME_RANGE_SINCE_UNIX_EPOCH: [Dayjs, Dayjs] = [
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

const DEFAULT_TIME_RANGE = TIME_RANGE_OPTION.ALL_TIME;

/* eslint-disable no-magic-numbers, @typescript-eslint/require-await */
const TIME_RANGE_OPTION_DAYJS_MAP: Record<
    TIME_RANGE_OPTION,
    (selectDataset: Nullable<string>) => Promise<[dayjs.Dayjs, dayjs.Dayjs]>
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
    [TIME_RANGE_OPTION.ALL_TIME]: async (selectDataset) => {
        let sql: string;
        if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
            sql = buildClpTimeRangeSql();
        } else {
            sql = buildClpsTimeRangeSql(selectDataset ?? "default");
        }
        const resp = await querySql<
            {
                begin_timestamp: Nullable<number>;
                end_timestamp: Nullable<number>;
            }[]
        >(sql);
        const [timestamps] = resp.data;
        if ("undefined" === typeof timestamps ||
              null === timestamps.begin_timestamp ||
              null === timestamps.end_timestamp
        ) {
            throw new Error("Unable to get All Time range");
        }

        return [
            dayjs.utc(timestamps.begin_timestamp),
            dayjs.utc(timestamps.end_timestamp),
        ];
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


export {
    DEFAULT_TIME_RANGE,
    isValidDateRange,
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
    TIME_RANGE_OPTION_NAMES,
    TIME_RANGE_SINCE_UNIX_EPOCH,
};
