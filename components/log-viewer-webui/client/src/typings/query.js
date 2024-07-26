/**
 * @typedef {number} QueryLoadState
 */
let enumQueryLoadState;
/**
 * Enum of query loading state.
 *
 * @enum {QueryLoadState}
 */
const QUERY_LOAD_STATE = Object.freeze({
    SUBMITTING: (enumQueryLoadState = 0),
    WAITING: ++enumQueryLoadState,
    LOADING: ++enumQueryLoadState,
});

/**
 * Descriptions for query states.
 */
const QUERY_STATE_DESCRIPTIONS = Object.freeze({
    [QUERY_LOAD_STATE.SUBMITTING]: {
        label: "Submitting query Job",
        description: "Parsing arguments and submitting job to the server.",
    },
    [QUERY_LOAD_STATE.WAITING]: {
        label: "Waiting for job to finish",
        description: "The job is running. Waiting for the job to finish.",
    },
    [QUERY_LOAD_STATE.LOADING]: {
        label: "Loading Log Viewer",
        description: "The query has been completed and the results are being loaded.",
    },
});

export {
    QUERY_LOAD_STATE,
    QUERY_STATE_DESCRIPTIONS,
};
