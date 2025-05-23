import dayjs from "dayjs";
import Duration from "dayjs/plugin/duration";


/**
 * The time range and bucket durations of the ResultsTimeline component.
 */
interface TimelineConfig {
    range: {begin: dayjs.Dayjs; end: dayjs.Dayjs};
    bucketDuration: Duration.Duration;
}

/**
 * The type of TooltipItem.raw in chart.js.
 */
interface ChartTooltipItemRaw {
    x: number;
}

export type {
    ChartTooltipItemRaw, TimelineConfig,
};
