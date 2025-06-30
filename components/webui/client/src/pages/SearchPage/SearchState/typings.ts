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
}

export {SEARCH_UI_STATE};
