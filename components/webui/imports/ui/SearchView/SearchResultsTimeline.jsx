import React from "react";
import {Bar} from "react-chartjs-2";

import {
    BarElement,
    Chart as ChartJs,
    LinearScale,
    TimeScale,
    Tooltip,
} from "chart.js";
import zoomPlugin from "chartjs-plugin-zoom";
import dayjs from "dayjs";

import {convertLocalUnixMsToSameUtcDatetime} from "./datetime";

import "chartjs-adapter-dayjs-4/dist/chartjs-adapter-dayjs-4.esm";
import "./SearchResultsTimeline.scss";


const DATETIME_FORMAT_TEMPLATE = "YYYY-MMM-DD HH:mm:ss";

ChartJs.register(
    TimeScale,
    LinearScale,
    BarElement,
    Tooltip,
    zoomPlugin
);

/**
 * Converts an array of timeline buckets into an array of objects compatible with Chart.js.
 *
 * @param {{timestamp: number, count: number}[]} timelineBuckets
 * @return {{x: number, y: number}[]}
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
 * Displays a timeline of search results.
 *
 * @param {TimelineConfig} timelineConfig
 * @param {object[]} timelineBuckets
 * @param {function} onTimelineZoom
 * @return {JSX.Element}
 */
const SearchResultsTimeline = ({
    timelineConfig,
    timelineBuckets,
    onTimelineZoom,
}) => {
    if (null === timelineBuckets) {
        return <></>;
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
                    parser: (date) => {
                        return dayjs.utc(date)
                            .tz(dayjs.tz.guess(), true)
                            .format();
                    },
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
                        const bucketStartTime = dayjs.utc(x);
                        const bucketEndTime = bucketStartTime
                            .add(timelineConfig.bucketDuration);

                        return `${bucketStartTime.format(DATETIME_FORMAT_TEMPLATE)} to\n` +
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
                        enabled: true,
                    },
                    mode: "x",
                    onZoom: ({chart}) => {
                        const xAxis = chart.scales.x;
                        const {min, max} = xAxis;
                        const newTimeRange = {
                            begin: convertLocalUnixMsToSameUtcDatetime(parseInt(min, 10)),
                            end: convertLocalUnixMsToSameUtcDatetime(parseInt(max, 10)),
                        };

                        onTimelineZoom(newTimeRange);
                    },
                },
            },
        },
    };

    return (
        <Bar
            data={data}
            id={"timeline-chart"}
            options={options}/>
    );
};

export default SearchResultsTimeline;
