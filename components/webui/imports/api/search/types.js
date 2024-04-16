/**
 * @typedef {object} TimeRange
 * @property {import("dayjs").Dayjs} begin
 * @property {import("dayjs").Dayjs} end
 */

/**
 * @typedef {object} TimelineConfig
 * @property {import("dayjs/plugin/duration").Duration} bucketDuration
 * @property {TimeRange} range
 */

/**
 * @typedef {object} TimelineBucket
 * @property {number} timestamp Timestamp as milliseconds since the Unix epoch.
 * @property {number} count
 */

/**
 * @typedef {object} ChartJsDatasetItem
 * @property {number} x
 * @property {number} y
 */
