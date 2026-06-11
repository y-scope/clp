import {SETTINGS_MAX_SEARCH_RESULTS} from "../../../../config";


/**
 * Padding for the table to the bottom of the page.
 */
const TABLE_BOTTOM_PADDING = 95;

/**
 * Hard cap for the default per-query result limit, ensuring safety regardless of system config.
 */
const SAFE_DEFAULT_LIMIT = 1_000;

/**
 * Default maximum number of results per query, clamped to protect against
 * aggressive system-wide MaxSearchResults configurations.
 */
const DEFAULT_SEARCH_MAX_NUM_RESULTS = Math.min(SAFE_DEFAULT_LIMIT, SETTINGS_MAX_SEARCH_RESULTS);

export {
    DEFAULT_SEARCH_MAX_NUM_RESULTS,
    TABLE_BOTTOM_PADDING,
};
