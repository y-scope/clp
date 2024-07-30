import axios from "axios";


/**
 * @typedef {object} ExtractIrResp
 * @property {number} begin_msg_ix
 * @property {number} end_msg_ix
 * @property {string} file_split_id
 * @property {boolean} is_last_ir_chunk
 * @property {string} orig_file_id
 * @property {string} path
 * @property {string} _id
 */

/**
 * Submits a job to extract the split of an original file that contains a given log event. The file
 * is extracted as a CLP IR file.
 *
 * @param {number|string} origFileId The ID of the original file
 * @param {number} logEventIdx The index of the log event
 * @param {Function} onUploadProgress Callback to handle upload progress events.
 * @return {Promise<axios.AxiosResponse<ExtractIrResp>>}
 */
const submitExtractIrJob = async (origFileId, logEventIdx, onUploadProgress) => {
    return await axios.post(
        "/query/extract-ir",
        {logEventIdx, origFileId},
        {onUploadProgress}
    );
};

export {submitExtractIrJob};
