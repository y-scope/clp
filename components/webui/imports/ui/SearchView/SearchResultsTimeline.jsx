import {
    convertLocalDateToSameUtcDatetime,
    convertUtcDatetimeToSameLocalDate,
    DATETIME_FORMAT_TEMPLATE,
    expandTimeRangeToDurationMultiple,
    TIME_UNIT,
} from "/imports/utils/datetime";

import "chartjs-adapter-dayjs-4/dist/chartjs-adapter-dayjs-4.esm";
import "./SearchResultsTimeline.scss";

import {BarElement, Chart as ChartJs, LinearScale, TimeScale, Tooltip} from "chart.js";
import zoomPlugin from "chartjs-plugin-zoom";
import dayjs from "dayjs";
import React from "react";
import {Bar} from "react-chartjs-2";
import {isDisablingUserInput} from "../../api/search/constants";


ChartJs.register(
    TimeScale,
    LinearScale,
    BarElement,
    Tooltip,
    zoomPlugin
);

const MAX_DATA_POINTS_PER_TIMELINE = 40;

/**
 * Converts an array of timeline buckets into an array of objects compatible with Chart.js.
 *
 * @param {TimelineBucket[]} timelineBuckets
 * @return {ChartJsDatasetItem[]}
 */
const adaptTimelineBucketsForChartJs = (timelineBuckets) => (
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
 * Converts a Chart.js timescale timestamp in Unix milliseconds to a UTC Dayjs object.
 * Due to the Chart.js timescale operating in the local timezone and the zoom plugin not
 * accounting for the effect of the parser during zoom operations, double reversions are required.
 *
 * @param {number} timestampUnixMillis
 * @return {dayjs.Dayjs} The corresponding Dayjs object
 */
const convertZoomTimestampToUtcDatetime = (timestampUnixMillis) => {
    // Create a Date object with given timestamp, which contains local timezone information.
    const initialDate = new Date(timestampUnixMillis);

    // Reverse local timezone offset
    const intermediateDateTime = convertLocalDateToSameUtcDatetime(initialDate);

    // Reverse local timezone offset again
    return convertLocalDateToSameUtcDatetime(intermediateDateTime.toDate());
}

/**
 * Computes the timestamp range and bucket duration necessary to render the bars in the timeline
 * chart.
 *
 * @param {dayjs.Dayjs} timestampBeginUnixMillis
 * @param {dayjs.Dayjs} timestampEndUnixMillis
 * @return {TimelineConfig}
 */
const computeTimelineConfig = (timestampBeginUnixMillis, timestampEndUnixMillis) => {
    const timeRangeMillis = timestampEndUnixMillis - timestampBeginUnixMillis;
    const exactTimelineBucketMillis = timeRangeMillis / MAX_DATA_POINTS_PER_TIMELINE;

    // A list of predefined bucket durations, ordered from least to greatest so that the
    // `durationSelections.find()` below can find the smallest bucket containing
    // `exactTimelineBucketMillis`.
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
            (duration) => (exactTimelineBucketMillis <= duration.asMilliseconds()),
        ) ||
        dayjs.duration(
            Math.ceil(exactTimelineBucketMillis / dayjs.duration(1, TIME_UNIT.YEAR).asMilliseconds()),
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

/**
 * Displays a timeline of search results.
 *
 * @param {object} resultsMetadata
 * @param {TimelineConfig} timelineConfig
 * @param {TimelineBucket[]} timelineBuckets
 * @param {function} onTimelineZoom
 * @return {JSX.Element}
 */
const SearchResultsTimeline = ({
    resultsMetadata,
    timelineConfig,
    timelineBuckets,
    onTimelineZoom,
}) => {
    if (null === timelineBuckets) {
        return <></>;
    }

    const isInputDisabled = isDisablingUserInput(resultsMetadata.lastSignal);

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

    const options = {
        animation: {
            duration: 100,
        },
        maintainAspectRatio: false,
        responsive: true,

        scales: {
            x: {
                type: "time",

                min: timelineConfig.range.begin.valueOf(),
                max: timelineConfig.range.end.valueOf(),
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
                    parser: (date) => convertUtcDatetimeToSameLocalDate(
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

                        return `${bucketBeginTime.format(DATETIME_FORMAT_TEMPLATE)} to\n` +
                            `${bucketEndTime.format(DATETIME_FORMAT_TEMPLATE)}`;
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
                        enabled: !isInputDisabled,
                        backgroundColor: "rgba(64,150,160,0.3)",
                    },
                    mode: "x",
                    onZoom: ({chart}) => {
                        const xAxis = chart.scales.x;
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
            className={isInputDisabled ?
                "timeline-chart-cursor-disabled" :
                ""}
            data={data}
            id={"timeline-chart"}
            options={options}/>
    );
};

export default SearchResultsTimeline;
export {computeTimelineConfig};