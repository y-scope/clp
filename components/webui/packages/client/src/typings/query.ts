import {Type} from "@sinclair/typebox";


/**
 * Enum for the different UI states of a query's loading process.
 */
enum QUERY_LOADING_STATE {
    // The query request is being formulated from user inputs.
    SUBMITTING = 0,

    // The request is being sent to the server, or it is sent and is pending response.
    WAITING,

    // The query is currently in the process of processing the response.
    LOADING,
}

/**
 * Values in enum `QUERY_LOADING_STATE`.
 */
const QUERY_LOADING_STATE_VALUES = Object.freeze(
    Object.values(QUERY_LOADING_STATE).filter((value) => "number" === typeof value)
);

/**
 * URL search parameters required for extract job.
 */
const ExtractJobSearchParams = Type.Object({
    archiveId: Type.String(),
    logEventIdx: Type.Number(),
    dataset: Type.Optional(Type.String()),
});

interface QueryLoadingStateDescription {
    label: string;
    description: string;
}

/**
 * Descriptions for query loading states.
 */
const QUERY_LOADING_STATE_DESCRIPTIONS: Record<QUERY_LOADING_STATE, QueryLoadingStateDescription> =
    Object.freeze({
        [QUERY_LOADING_STATE.SUBMITTING]: {
            label: "Submitting query Job",
            description: "Parsing arguments and submitting job to the server.",
        },
        [QUERY_LOADING_STATE.WAITING]: {
            label: "Waiting for job to finish",
            description: "The job is running. Waiting for the job to finish.",
        },
        [QUERY_LOADING_STATE.LOADING]: {
            label: "Loading Log Viewer",
            description: "The query has been completed and the results are being loaded.",
        },
    });


export {
    ExtractJobSearchParams,
    QUERY_LOADING_STATE,
    QUERY_LOADING_STATE_DESCRIPTIONS,
    QUERY_LOADING_STATE_VALUES,
};
