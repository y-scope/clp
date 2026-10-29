import dayjs, {Dayjs} from "dayjs";
import DayjsTimezone from "dayjs/plugin/timezone";
import DayjsUtc from "dayjs/plugin/utc";


dayjs.extend(DayjsUtc);
dayjs.extend(DayjsTimezone);

/**
 * Converts a UTC Dayjs object to a local-timezone JavaScript Date object that represents the same
 * date and time. In other words, the original year, month, day, hour, minute, second, and
 * millisecond appear unchanged.
 *
 * @param utcDatetime
 * @return The corresponding Date object
 */
const convertUtcDatetimeToSameLocalDate = (utcDatetime: dayjs.Dayjs): Date => {
    const localTz = dayjs.tz.guess();
    return utcDatetime.tz(localTz, true).toDate();
};

/**
 * Converts a local-timezone JavaScript Date object to a UTC Dayjs object that represents the same
 * date and time. In other words, the original year, month, day, hour, minute, second, and
 * millisecond appear unchanged.
 *
 * @param localDate
 * @return The corresponding Dayjs object
 */
const convertLocalDateToSameUtcDatetime = (localDate: Date) => {
    return dayjs(localDate).utc(true);
};

/**
 * Converts the timestamp from Chart.js' zoom plugin to a UTC Dayjs object.
 * NOTE: The Chart.js timescale operates in the local timezone, but we want to the timeline to
 * appear as if it's in UTC, so we apply the negative offset of the local timezone to all timestamps
 * before passing them to Chart.js. However, the zoom plugin thinks that Chart.js is displaying
 * timestamps in the local timezone, so it also applies the negative offset of the local timezone
 * before passing them to onZoom. So to get the original UTC timestamp, this method needs to apply
 * the local timezone offset twice.
 *
 * @param timestampUnixMillis
 * @return The corresponding Dayjs object
 */
const convertZoomTimestampToUtcDatetime = (timestampUnixMillis: number): Dayjs => {
    // Create a Date object with given timestamp, which contains local timezone information.
    const initialDate = new Date(timestampUnixMillis);

    // Reverse local timezone offset.
    const intermediateDateTime = convertLocalDateToSameUtcDatetime(initialDate);

    // Reverse local timezone offset again.
    return convertLocalDateToSameUtcDatetime(intermediateDateTime.toDate());
};

export {
    convertLocalDateToSameUtcDatetime,
    convertUtcDatetimeToSameLocalDate,
    convertZoomTimestampToUtcDatetime,
};
