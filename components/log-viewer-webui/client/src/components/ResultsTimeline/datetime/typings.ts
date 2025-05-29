import {Dayjs} from "dayjs";


// eslint-disable-next-line no-warning-comments
// TODO: replace `TimeRange` with [dayjs.Dayjs, dayjs.Dayjs]
// to keep consistent with `TimeRangeInput`.
/**
 * Begin and end dates for the results timeline.
 */
type TimeRange = {
    begin: Dayjs;
    end: Dayjs;
};

export type {TimeRange};
