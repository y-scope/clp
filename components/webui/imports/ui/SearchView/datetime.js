import dayjs from "dayjs";

const DATETIME_FORMAT_TEMPLATE = "MMM DD, YYYY, HH:mm:ss";

const TIME_UNIT = Object.freeze({
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
 * @returns {Object} containing Date objects representing the computed begin and end time range
 */
const computeTimeRange = (token) => () => {
    const [unit,
        modifier,
        amount] = token.split("_");
    let end;
    let begin;

    if (TIME_UNIT.ALL === unit) {
        end = dayjs.utc().add(1, "year");
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
            dayjs.utc().subtract(amount, unit)
                .endOf(unit);
        console.log(end);
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

// TODO: Switch date pickers so we don't have to do this hack
/**
 * Converts a Dayjs object in UTC, to a JavaScript Date object that represents the same date and
 * time, but aligned with the local timezone.
 * This is achieved without altering the original timestamp values (year, month, day, hour,
 * minute, second, and millisecond).
 *
 * @param {dayjs.Dayjs} utcDatetime
 * @return {Date | undefined} The corresponding Date object, or `undefined` when `utcDatetime`
 * is `undefined`.
 */
const convertUtcToSameLocalDate = (utcDatetime) => {
    const localTz = dayjs.tz.guess();
    return utcDatetime?.tz(localTz, true).toDate();
};

/**
 * Converts a JavaScript Date object in local timezone, to a Dayjs object that represents the same
 * date and time, but aligned with the UTC.
 *
 * @param {Date} localDate
 * @return {dayjs.Dayjs} The corresponding Dayjs object
 */
const convertLocalToSameUtcDatetime = (localDate) => {
    return dayjs(localDate)?.utc(true);
};

/**
 * Converts a Unix milliseconds timestamp of a JavaScript Date object in local timezone, to a Dayjs
 * object that represents the same date and time, but aligned with the UTC.
 *
 * @param {number} localTimeStampUnixMs
 * @return {dayjs.Dayjs} The corresponding Dayjs object
 */
const convertLocalUnixMsToSameUtcDatetime = (localTimeStampUnixMs) => {
    const localTz = dayjs.tz.guess();

    return dayjs(localTimeStampUnixMs)?.tz(localTz)
        .utc(true)
        .tz(localTz)
        .utc(true);
};

/**
 * Clip the time range to the nearest timeline bucket.
 *
 * @param {dayjs.Duration} bucketDuration - The `dayjs.Duration` of each timeline bucket.
 * @param {Object} timeRange - The time range to be clipped.
 * @param {dayjs.Dayjs} timeRange.begin
 * @param {dayjs.Dayjs} timeRange.end
 * @return {Object} - The clipped time range.
 * @property {dayjs.Dayjs} begin
 * @property {dayjs.Dayjs} end
 */
const clipTimeRangeToTimelineBucket = (bucketDuration, {
    begin,
    end,
}) => {
    const adjustedBegin =
        begin - (begin % bucketDuration.asMilliseconds());
    const adjustedEnd =
        Math.floor(
            (end + bucketDuration.asMilliseconds() - 1) /
            bucketDuration.asMilliseconds()
        ) * bucketDuration.asMilliseconds();

    return {begin: dayjs.utc(adjustedBegin), end: dayjs.utc(adjustedEnd)};
};

/**
 * Computes timeline configuration based on the given timestamp range.
 *
 * @param {dayjs.Dayjs} timestampBeginUnixMs
 * @param {dayjs.Dayjs} timestampEndUnixMs
 * @return {Object} - The computed timeline configuration.
 */
const computeTimelineConfig = (timestampBeginUnixMs, timestampEndUnixMs) => {
    const timeRangeMs = timestampEndUnixMs - timestampBeginUnixMs;
    const maxDataPointsPerTimeline = 40;
    const exactTimelineBucketMs = timeRangeMs / maxDataPointsPerTimeline;

    const durationSelections = [
        /* eslint-disable @stylistic/js/array-element-newline, no-magic-numbers */
        {unit: "second", values: [1, 2, 5, 10, 15, 30]},
        {unit: "minute", values: [1, 2, 5, 10, 15, 20, 30]},
        {unit: "hour", values: [1, 2, 3, 4, 8, 12]},
        {unit: "day", values: [1, 2, 5, 15]},
        {unit: "month", values: [1, 2, 3, 4, 6]},
        {unit: "year", values: [1]},
        /* eslint-enable @stylistic/js/array-element-newline, no-magic-numbers */
    ].flatMap(
        ({
            unit,
            values,
        }) => values.map(
            (value) => dayjs.duration(value, unit),
        ),
    );

    const bucketDuration =
        durationSelections.find(
            (duration) => (exactTimelineBucketMs <= duration.asMilliseconds()),
        ) ||
        dayjs.duration(
            Math.ceil(exactTimelineBucketMs / dayjs.duration(1, "year").asMilliseconds()),
            "year",
        );

    return {
        range: clipTimeRangeToTimelineBucket(bucketDuration, {
            begin: dayjs.utc(timestampBeginUnixMs),
            end: dayjs.utc(timestampEndUnixMs),
        }),
        bucketDuration: bucketDuration,
    };
};

export {
    clipTimeRangeToTimelineBucket,
    computeTimelineConfig,
    computeTimeRange,
    convertLocalToSameUtcDatetime,
    convertLocalUnixMsToSameUtcDatetime,
    convertUtcToSameLocalDate,
    DATETIME_FORMAT_TEMPLATE,
    DEFAULT_TIME_RANGE,
    TIME_RANGE_PRESET_LABEL,
};
