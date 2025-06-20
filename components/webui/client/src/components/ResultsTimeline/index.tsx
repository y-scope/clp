import {useEffect} from "react";
import {Bar} from "react-chartjs-2";

import {theme} from "antd";
import {
    BarElement,
    Chart,
    Chart as ChartJs,
    type ChartOptions,
    LinearScale,
    TimeScale,
    Tooltip,
    TooltipItem,
} from "chart.js";
import zoomPlugin from "chartjs-plugin-zoom";
import dayjs, {Dayjs} from "dayjs";

import {DATETIME_FORMAT_TEMPLATE} from "../../typings/datetime";
import {
    convertUtcDatetimeToSameLocalDate,
    convertZoomTimestampToUtcDatetime,
} from "./datetime";
import styles from "./index.module.css";
import {
    TimelineBucket,
    TimelineConfig,
} from "./typings";
import {
    adaptTimelineBucketsForChartJs,
    deselectAll,
} from "./utils";

import "chartjs-adapter-dayjs-4/dist/chartjs-adapter-dayjs-4.esm";


ChartJs.register(
    TimeScale,
    LinearScale,
    BarElement,
    Tooltip,
    zoomPlugin
);

interface ResultsTimelineProps {
    isInputDisabled: boolean;
    onTimelineZoom: (newTimeRange: [Dayjs, Dayjs]) => void;
    timelineBuckets: TimelineBucket[];
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
// eslint-disable-next-line max-lines-per-function
const ResultsTimeline = ({
    isInputDisabled,
    onTimelineZoom,
    timelineBuckets,
    timelineConfig,
}: ResultsTimelineProps) => {
    const {token} = theme.useToken();

    useEffect(() => {
        document.documentElement.style.setProperty(
            "--timeline-chart-cursor",
            isInputDisabled ?
                "wait" :
                "crosshair"
        );
    }, [isInputDisabled]);

    const data = {
        datasets: [
            {
                backgroundColor: token.colorPrimary,
                barPercentage: 1.2,
                minBarLength: 5,

                data: adaptTimelineBucketsForChartJs(timelineBuckets),
            },
        ],
    };

    Chart.defaults.font.family = token.fontFamily;
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
                    parser: (date: unknown) => convertUtcDatetimeToSameLocalDate(
                        dayjs.utc(date as number)
                    ).getTime(),
                },
            },
            y: {
                suggestedMax: 4,
                type: "linear",
                ticks: {
                    precision: 0,
                    autoSkip: true,
                    autoSkipPadding: 10,
                },
            },
        },

        plugins: {
            tooltip: {
                callbacks: {
                    title: (tooltipItems) => {
                        const [firstTooltipItem]: TooltipItem<"bar">[] = tooltipItems;
                        if ("undefined" === typeof firstTooltipItem) {
                            return "";
                        }
                        const {x} = firstTooltipItem.raw as {x: number};
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
                        backgroundColor: token.colorBgTextActive,
                    },
                    mode: "x",
                    onZoom: ({chart}) => {
                        const xAxis = chart.scales["x"];
                        if ("undefined" === typeof xAxis) {
                            return;
                        }
                        const {min, max} = xAxis;
                        const newTimeRange: [Dayjs, Dayjs] = [
                            convertZoomTimestampToUtcDatetime(min),
                            convertZoomTimestampToUtcDatetime(max),
                        ];

                        onTimelineZoom(newTimeRange);
                    },
                },
            },
        },
    };

    return (
        <Bar
            className={styles["timelineChart"]}
            data={data}
            options={options}

            // If the user inadvertently selected the timeline, then dragging on it will drag the
            // timeline object rather than selecting a time range within it. Thus, on mouse down, we
            // clear any existing selections so that the following drag selects a time range.
            onMouseDown={deselectAll}/>
    );
};

export default ResultsTimeline;
