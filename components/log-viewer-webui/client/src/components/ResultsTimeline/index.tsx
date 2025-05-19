import {useEffect} from "react";
import {Bar} from "react-chartjs-2";

import {
    BarElement,
    Chart as ChartJs,
    type ChartOptions,
    LinearScale,
    TimeScale,
    Tooltip,
} from "chart.js";
import zoomPlugin from "chartjs-plugin-zoom";
import dayjs from "dayjs";
import Duration, {DurationUnitType} from "dayjs/plugin/duration";

import {Nullable} from "../../typings/common";
import {
    convertLocalDateToSameUtcDatetime,
    convertUtcDatetimeToSameLocalDate,
    DATETIME_FORMAT_TEMPLATE,
    expandTimeRangeToDurationMultiple,
    TIME_UNIT,
    TimeRange,
} from "../../utils/datetime";
import {deselectAll} from "../../utils/misc";

import "chartjs-adapter-dayjs-4/dist/chartjs-adapter-dayjs-4.esm";
import "./ResultsTimeline.css";


ChartJs.register(
    TimeScale,
    LinearScale,
    BarElement,
    Tooltip,
    zoomPlugin
);


interface TimelineBucket {
    timestamp: number;
    count: number;
}

const MAX_DATA_POINTS_PER_TIMELINE = 40;

/**
 * Converts an array of timeline buckets into an array of objects compatible with Chart.js.
 *
 * @param timelineBuckets
 * @return
 */
const adaptTimelineBucketsForChartJs = (timelineBuckets: TimelineBucket[]) => (
    timelineBuckets.map(
        ({
            timestamp,
            count,
        }) => ({
            x: timestamp,
            y: count,
        })
    )
);

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
const convertZoomTimestampToUtcDatetime = (timestampUnixMillis: number) => {
    // Create a Date object with given timestamp, which contains local timezone information.
    const initialDate = new Date(timestampUnixMillis);

    // Reverse local timezone offset.
    const intermediateDateTime = convertLocalDateToSameUtcDatetime(initialDate);

    // Reverse local timezone offset again.
    return convertLocalDateToSameUtcDatetime(intermediateDateTime.toDate());
};

interface TimelineConfig {
    range: {begin: dayjs.Dayjs; end: dayjs.Dayjs};
    bucketDuration: Duration.Duration;
}

/**
 * Computes the timestamp range and bucket duration necessary to render the bars in the timeline
 * chart.
 *
 * @param timestampBeginUnixMillis
 * @param timestampEndUnixMillis
 * @return
 */
const computeTimelineConfig = (
    timestampBeginUnixMillis: number,
    timestampEndUnixMillis: number
): TimelineConfig => {
    const timeRangeMillis = timestampEndUnixMillis - timestampBeginUnixMillis;
    const exactTimelineBucketMillis = timeRangeMillis / MAX_DATA_POINTS_PER_TIMELINE;

    // A list of predefined bucket durations, ordered from least to greatest so that the
    // `durationSelections.find()` below can find the smallest bucket containing
    // `exactTimelineBucketMillis`.
    const durationSelections = [
        /* eslint-disable @stylistic/array-element-newline, no-magic-numbers */
        {unit: "second", values: [1, 2, 5, 10, 15, 30]},
        {unit: "minute", values: [1, 2, 5, 10, 15, 20, 30]},
        {unit: "hour", values: [1, 2, 3, 4, 8, 12]},
        {unit: "day", values: [1, 2, 5, 15]},
        {unit: "month", values: [1, 2, 3, 4, 6]},
        {unit: "year", values: [1]},
        /* eslint-enable @stylistic/array-element-newline, no-magic-numbers */
    ].flatMap(
        ({
            unit,
            values,
        }) => values.map(
            (value) => dayjs.duration(value, unit as DurationUnitType),
        ),
    );

    const bucketDuration =
        durationSelections.find(
            (duration) => (exactTimelineBucketMillis <= duration.asMilliseconds()),
        ) ||
        dayjs.duration(
            Math.ceil(exactTimelineBucketMillis /
                dayjs.duration(1, TIME_UNIT.YEAR).asMilliseconds()),
            TIME_UNIT.YEAR,
        );

    return {
        range: expandTimeRangeToDurationMultiple(bucketDuration, {
            begin: dayjs.utc(timestampBeginUnixMillis),
            end: dayjs.utc(timestampEndUnixMillis),
        }),
        bucketDuration: bucketDuration,
    };
};

interface SearchResultsTimelineProps {
    isInputDisabled: boolean;
    onTimelineZoom: (newTimeRange: TimeRange) => void;
    timelineBuckets: Nullable<TimelineBucket[]>;
    timelineConfig: TimelineConfig;
}

/**
 * Displays a timeline of search results.
 *
 * @param props
 * @param props.isInputDisabled
 * @param props.onTimelineZoom
 * @param props.timelineBuckets
 * @param props.timelineConfig
 * @return
 */
const SearchResultsTimeline = ({
    isInputDisabled,
    onTimelineZoom,
    timelineBuckets,
    timelineConfig,
}: SearchResultsTimelineProps) => {
    useEffect(() => {
        document.documentElement.style.setProperty(
            "--timeline-chart-cursor",
            isInputDisabled ?
                "wait" :
                "crosshair"
        );
    }, [isInputDisabled]);

    if (null === timelineBuckets) {
        return <div/>;
    }

    const data = {
        datasets: [
            {
                backgroundColor: "#4096a0",
                barPercentage: 1.2,
                borderColor: "#007380",
                borderWidth: 2,
                minBarLength: 5,

                data: adaptTimelineBucketsForChartJs(timelineBuckets),
            },
        ],
    };

    const options: ChartOptions<"bar"> = {
        animation: {
            duration: 100,
        },
        maintainAspectRatio: false,
        responsive: true,

        scales: {
            x: {
                type: "time",

                max: timelineConfig.range.end.valueOf(),
                min: timelineConfig.range.begin.valueOf(),
                offset: false,

                grid: {
                    drawOnChartArea: false,
                    color: "black",
                    drawTicks: true,
                    offset: true,
                },
                ticks: {
                    maxRotation: 0,
                    autoSkipPadding: 30,
                    major: {enabled: true},
                },
                time: {
                    displayFormats: {
                        millisecond: "HH:mm:ss.SSS",
                        second: "HH:mm:ss",
                        minute: "HH:mm",
                        hour: "HH:mm",
                    },
                    parser: (date: number) => convertUtcDatetimeToSameLocalDate(
                        dayjs.utc(date)
                    ),
                },
            },
            y: {
                ticks: {
                    autoSkip: true,
                    autoSkipPadding: 10,
                },
            },
        },

        plugins: {
            tooltip: {
                callbacks: {
                    title: (tooltipItems) => {
                        const [{raw: {x}}] = tooltipItems;
                        const bucketBeginTime = dayjs.utc(x);
                        const bucketEndTime = bucketBeginTime
                            .add(timelineConfig.bucketDuration);

                        return `${bucketBeginTime.format(DATETIME_FORMAT_TEMPLATE)} to\n${
                            bucketEndTime.format(DATETIME_FORMAT_TEMPLATE)}`;
                    },
                },
                caretSize: 0,
                intersect: false,
                mode: "x",
                xAlign: "left",
                yAlign: "bottom",
            },
            zoom: {
                zoom: {
                    drag: {
                        enabled: false === isInputDisabled,
                        backgroundColor: "rgba(64,150,160,0.3)",
                    },
                    mode: "x",
                    onZoom: ({chart}) => {
                        const xAxis = chart.scales["x"];
                        const {min, max} = xAxis;
                        const newTimeRange = {
                            begin: convertZoomTimestampToUtcDatetime(parseInt(min, 10)),
                            end: convertZoomTimestampToUtcDatetime(parseInt(max, 10)),
                        };

                        onTimelineZoom(newTimeRange);
                    },
                },
            },
        },
    };

    return (
        <Bar
            className={"timeline-chart"}
            data={data}
            options={options}

            // If the user inadvertently selected the timeline, then dragging on it will drag the
            // timeline object rather than selecting a time range within it. Thus, on mouse down, we
            // clear any existing selections so that the following drag selects a time range.
            onMouseDown={deselectAll}/>
    );
};

export type {
    TimelineBucket,
    TimelineConfig,
};
export default SearchResultsTimeline;
export {computeTimelineConfig};
