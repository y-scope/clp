import axios, {AxiosError} from "axios";


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

/**
 * Submits a job to extract the split of an original file that contains a given log event. The file
 * is extracted as a CLP IR file.
 *
 * @param {number|string} origFileId The ID of the original file
 * @param {number} logEventIdx The index of the log event
 * @param {Function} onQueryStateChange Callback to set query state.
 * @param {Function} onErrorMsg Callback to set error message.
 */
const submitExtractIrJob = async (origFileId, logEventIdx, onQueryStateChange, onErrorMsg) => {
    try {
        const {data} = await axios.post("/query/extract-ir", {logEventIdx, origFileId});
        onQueryStateChange(QUERY_LOAD_STATE.LOADING);

        const innerLogEventNum = logEventIdx - data.begin_msg_ix + 1;
        window.location = `/log-viewer/index.html?filePath=/ir/${data.path}` +
            `#logEventIdx=${innerLogEventNum}`;
    } catch (e) {
        let errorMsg = "Unknown error.";
        if (e instanceof AxiosError) {
            errorMsg = e.message;
            if ("undefined" !== typeof e.response) {
                if ("undefined" !== typeof e.response.data.message) {
                    errorMsg = e.response.data.message;
                } else {
                    errorMsg = e.response.statusText;
                }
            }
        }
        console.error(errorMsg, e);
        onErrorMsg(errorMsg);
    }
};

export {
    QUERY_LOAD_STATE,
    QUERY_STATE_DESCRIPTIONS,
    submitExtractIrJob,
};
