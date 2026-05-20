/**
 * Available options for the maximum number of search results.
 */
/* eslint-disable no-magic-numbers */
const MAX_RESULTS_OPTIONS = [
    10,
    50,
    100,
    500,
    1000,
    5000,
    10000,
] as const;
/* eslint-enable no-magic-numbers */

/**
 * Default value for the maximum number of search results.
 */
const DEFAULT_MAX_NUM_RESULTS: number = 1000;

/**
 * Search UI states.
 */
enum SEARCH_UI_STATE {
    /**
     * Default state when client starts and there is no active query.
     */
    DEFAULT,

    /**
     * When query is submitted, but the server has not yet responded with a query ID.
     */
    QUERY_ID_PENDING,

    /**
     * After the client recieved the query ID, and the query is being processed on sever.
     */
    QUERYING,

    /**
     * When the query is complete or cancelled.
     */
    DONE,

    /**
     * When the query failed due to an error.
     */
    FAILED,
}

export {
    DEFAULT_MAX_NUM_RESULTS,
    MAX_RESULTS_OPTIONS,
    SEARCH_UI_STATE,
};
