/**
 * @typedef {object} TimeRange
 * @property {dayjs.Dayjs} begin
 * @property {dayjs.Dayjs} end
 */

/**
 * @typedef {object} TimelineConfig
 * @property {plugin.Duration} bucketDuration
 * @property {TimeRange} range
 */

/**
 * @typedef {object} TimelineBucket
 * @property {number} timestamp
 * @property {number} count
 */

/**
 * @typedef {object} ChartJsDatasetItem
 * @property {number} x
 * @property {number} y
 */
