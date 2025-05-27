
/**
 * Enum representing the different states of the search UI.
 */
enum SEARCH_UI_STATE {
    /**
     * Default state when client starts.
     */
    DEFAULT,
    /**
     * When query is submitted, but the query ID is not yet available.
     */
    QUERY_ID_PENDING,
    /**
     * When the query is being processed.
     */
    QUERYING,
    /**
     * When the query is finished or cancelled.
     */
    DONE,
}

export {SEARCH_UI_STATE};