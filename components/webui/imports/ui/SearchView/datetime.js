import {DateTime} from "luxon";

export const computeTodayTimeRange = () => {
    const endTime = DateTime.utc();
    const beginTime = endTime.minus({days: 1});
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computeWeekToDateTimeRange = () => {
    const endTime = DateTime.utc();
    const beginTime = endTime.startOf("week");
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computeMonthToDateTimeRange = () => {
    const endTime = DateTime.utc();
    const beginTime = endTime.startOf("month");
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computeYearToDateTimeRange = () => {
    const endTime = DateTime.utc();
    const beginTime = endTime.startOf("year");
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computePrevDayTimeRange = () => {
    const endTime = DateTime.utc().minus({days: 1}).endOf("day");
    const beginTime = endTime.startOf("day");
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computePrevWeekTimeRange = () => {
    const endTime = DateTime.utc().minus({weeks: 1}).endOf("week");
    const beginTime = endTime.startOf("week");
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computePrevMonthTimeRange = () => {
    const endTime = DateTime.utc().minus({months: 1}).endOf("month");
    const beginTime = endTime.startOf("month");
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computePrevYearTimeRange = () => {
    const endTime = DateTime.utc().minus({years: 1}).endOf("year");
    const beginTime = endTime.startOf("year");
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computeLast15MinTimeRange = () => {
    const endTime = DateTime.utc();
    const beginTime = endTime.minus({minutes: 15});
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computeLast60MinTimeRange = () => {
    const endTime = DateTime.utc();
    const beginTime = endTime.minus({minutes: 60});
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computeLast4HourTimeRange = () => {
    const endTime = DateTime.utc();
    const beginTime = endTime.minus({hours: 4});
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computeLast24HourTimeRange = () => {
    const endTime = DateTime.utc();
    const beginTime = endTime.minus({hours: 24});
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const computeAllTimeRange = () => {
    const endTime = DateTime.utc().plus({years: 1});
    const beginTime = DateTime.fromMillis(0, {zone: "UTC"});
    return {
        begin: dateTimeToDateWithoutChangingTimestamp(beginTime),
        end: dateTimeToDateWithoutChangingTimestamp(endTime),
    };
};

export const cTimePresets = [
    {
        key: "last-15-mins",
        label: "Last 15 Minutes",
        compute: computeLast15MinTimeRange,
    },
    {
        key: "last-60-mins",
        label: "Last 60 Minutes",
        compute: computeLast60MinTimeRange,
    },
    {
        key: "last-4-hours",
        label: "Last 4 Hours",
        compute: computeLast4HourTimeRange,
    },
    {
        key: "last-24-hours",
        label: "Last 24 Hours",
        compute: computeLast24HourTimeRange,
    },
    {
        key: "prev-day",
        label: "Previous Day",
        compute: computePrevDayTimeRange,
    },
    {
        key: "prev-week",
        label: "Previous Week",
        compute: computePrevWeekTimeRange,
    },
    {
        key: "prev-month",
        label: "Previous Month",
        compute: computePrevMonthTimeRange,
    },
    {
        key: "prev-year",
        label: "Previous Year",
        compute: computePrevYearTimeRange,
    },
    {
        key: "today",
        label: "Today",
        compute: computeTodayTimeRange,
    },
    {
        key: "week-to-date",
        label: "Week to Date",
        compute: computeWeekToDateTimeRange,
    },
    {
        key: "month-to-date",
        label: "Month to Date",
        compute: computeMonthToDateTimeRange,
    },
    {
        key: "year-to-date",
        label: "Year to Date",
        compute: computeYearToDateTimeRange,
    },
    {
        key: "all-time",
        label: "All Time",
        compute: computeAllTimeRange,
    },
];

export const changeTimezoneToUTCWithoutChangingTime = (date) => {
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

// TODO Switch date pickers so we don't have to do this hack
export const dateTimeToDateWithoutChangingTimestamp = (dateTime) => {
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
