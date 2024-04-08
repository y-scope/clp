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

import {
    convertLocalUnixMsToSameUtcDatetime,
    DATETIME_FORMAT_TEMPLATE,
} from "/imports/utils/datetime";

import "chartjs-adapter-dayjs-4/dist/chartjs-adapter-dayjs-4.esm";
import "./SearchResultsTimeline.scss";
import {isDisablingUserInput} from "../../api/search/constants";


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
            className={isInputDisabled ?
                "timeline-chart-cursor-disabled" :
                ""}
            data={data}
            id={"timeline-chart"}
            options={options}/>
    );
};

export default SearchResultsTimeline;
