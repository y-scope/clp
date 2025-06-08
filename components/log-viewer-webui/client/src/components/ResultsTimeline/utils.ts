import {TimelineBucket} from "./typings";


/**
 * Converts an array of timeline buckets into an array of objects compatible with Chart.js.
 *
 * @param timelineBuckets
 * @return
 */
const adaptTimelineBucketsForChartJs = (timelineBuckets: TimelineBucket[])
: {x: number; y: number}[] => (
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
 * Deselects all selections within the browser viewport.
 */
const deselectAll = () => {
    const selection = window.getSelection();
    if (null !== selection) {
        selection.removeAllRanges();
    }
};

export {
    adaptTimelineBucketsForChartJs,
    deselectAll,
};
