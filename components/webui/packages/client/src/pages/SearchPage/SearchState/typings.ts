import {
    DEFAULT_MAX_NUM_SEARCH_RESULTS,
    MAX_NUM_SEARCH_RESULTS,
} from "@webui/common/schemas/search";


/**
 * Available options for the maximum number of search results.
 */
/* eslint-disable no-magic-numbers */
const MAX_RESULTS_OPTIONS = [
    10,
    50,
    100,
    500,
    DEFAULT_MAX_NUM_SEARCH_RESULTS,
    5000,
    MAX_NUM_SEARCH_RESULTS,
] as const;
/* eslint-enable no-magic-numbers */

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
    MAX_RESULTS_OPTIONS,
    SEARCH_UI_STATE,
};
