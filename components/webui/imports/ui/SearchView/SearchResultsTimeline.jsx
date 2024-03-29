import React from "react";
import {Bar} from "react-chartjs-2";

import {
    clipTimeRangeToTimelineBucket,
    convertLocalUnixMsToSameUtcDatetime,
    DATETIME_FORMAT_TEMPLATE,
} from "./datetime";

import "./SearchResultsTimeline.scss";
import dayjs from "dayjs";


/**
 * Converts an array of timeline buckets into an array of objects compatible with Chart.js.
 *
 * @param {object[]} timelineBuckets
 * @return {object[]}
 */
const getChartJsDateFrom = (timelineBuckets) => {
    return timelineBuckets.map(({timestamp, count}) => ({
        x: Number(timestamp),
        y: count,
    }));
};

/**
 * Displays a timeline of search results.
 *
 * @param {object} timelineConfig
 * @param {object[]} timelineBuckets
 * @param {function} onSubmitQuery
 * @return {JSX.Element}
 */
const SearchResultsTimeline = ({
    timelineConfig,
    timelineBuckets,
    onSubmitQuery,
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
                data: getChartJsDateFrom(timelineBuckets),
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

                        return `  ${bucketStartTime.format(DATETIME_FORMAT_TEMPLATE)}\n` +
                            `- ${bucketEndTime.format(DATETIME_FORMAT_TEMPLATE)}`;
                    },

                },
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

                        onSubmitQuery(clipTimeRangeToTimelineBucket(
                            timelineConfig.bucketDuration,
                            newTimeRange
                        ));
                    },
                },
            },
        },
    };

    return (
        <>
            <Bar
                data={data}
                id={"timeline-chart"}
                options={options}/>
            <div id={"timeline-timestamp-begin-text"}>
                {dayjs.utc(timelineConfig.range.begin).format(DATETIME_FORMAT_TEMPLATE)}
            </div>
        </>
    );
};

export default SearchResultsTimeline;
