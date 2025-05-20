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

export type {TimelineBucket};
export {
    adaptTimelineBucketsForChartJs, MAX_DATA_POINTS_PER_TIMELINE,
};
