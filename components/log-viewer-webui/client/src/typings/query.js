/**
 * @typedef {number} QueryLoadingState
 */
let enumQueryLoadingState;
/**
 * Enum of query loading state.
 *
 * @enum {QueryLoadingState}
 */
const QUERY_LOADING_STATES = Object.freeze({
    SUBMITTING: (enumQueryLoadingState = 0),
    WAITING: ++enumQueryLoadingState,
    LOADING: ++enumQueryLoadingState,
});

/**
 * Descriptions for query loading states.
 */
const QUERY_LOADING_STATE_DESCRIPTIONS = Object.freeze({
    [QUERY_LOADING_STATES.SUBMITTING]: {
        label: "Submitting query Job",
        description: "Parsing arguments and submitting job to the server.",
    },
    [QUERY_LOADING_STATES.WAITING]: {
        label: "Waiting for job to finish",
        description: "The job is running. Waiting for the job to finish.",
    },
    [QUERY_LOADING_STATES.LOADING]: {
        label: "Loading Log Viewer",
        description: "The query has been completed and the results are being loaded.",
    },
});

export {
    QUERY_LOADING_STATE_DESCRIPTIONS,
    QUERY_LOADING_STATES,
};
