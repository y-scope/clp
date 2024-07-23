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
 * Submits a job to extract a segment of the original file, which contains a given log event index,
 * into CLP's IR format for viewing in the Log Viewer.
 *
 * @param {number|string} origFileId The ID of the original file to extract IR from
 * @param {number} logEventIx The index of the log event
 * @param {Function} onQueryStateChange Callback to set query state.
 * @param {Function} onErrorMsg Callback to set error message.
 */
const submitExtractIrJob = async (origFileId, logEventIx, onQueryStateChange, onErrorMsg) => {
    try {
        const {data} = await axios.post("/query/extract-ir", {
            msg_ix: logEventIx,
            orig_file_id: origFileId,
        });

        onQueryStateChange(QUERY_LOAD_STATE.LOADING);

        window.location = `/log-viewer/index.html?filePath=/ir/${data.path}`;
    } catch (e) {
        let errorMsg = "Unknown error.";
        if (e instanceof AxiosError) {
            errorMsg = e.message;
            if ("undefined" !== typeof e.response) {
                errorMsg = e.response.data.message ?? e.response.statusText;
            }
        }
        onErrorMsg(errorMsg);
    }
};

export {
    QUERY_LOAD_STATE,
    submitExtractIrJob,
};
