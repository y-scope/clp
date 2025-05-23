import dayjs from "dayjs";
import Duration from "dayjs/plugin/duration";


interface TimelineConfig {
    range: {begin: dayjs.Dayjs; end: dayjs.Dayjs};
    bucketDuration: Duration.Duration;
}


interface ChartTooltipItemRaw {
    x: number;
}

export type {
    ChartTooltipItemRaw, TimelineConfig,
};
