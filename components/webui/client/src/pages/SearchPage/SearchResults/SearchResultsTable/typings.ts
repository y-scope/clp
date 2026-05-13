import {SETTINGS_MAX_SEARCH_RESULTS} from "../../../../config";


/**
 * Padding for the table to the bottom of the page.
 */
const TABLE_BOTTOM_PADDING = 95;

/**
 * Default maximum number of results per query, clamped to protect against
 * aggressive system-wide MaxSearchResults configurations.
 */
const DEFAULT_SEARCH_MAX_NUM_RESULTS = Math.min(1000, SETTINGS_MAX_SEARCH_RESULTS);

export {
    DEFAULT_SEARCH_MAX_NUM_RESULTS,
    TABLE_BOTTOM_PADDING,
};
