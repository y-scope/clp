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
 * @typedef {object} ExtractJsonResp
 * @property {number} begin_msg_ix
 * @property {number} end_msg_ix
 * @property {boolean} is_last_ir_chunk
 * @property {string} orig_file_id
 * @property {string} path
 * @property {string} _id
 */

/**
 * Submits a job to extract the stream that contains a given log event. The stream is extracted
 * either as a CLP IR or a JSON Lines file.
 *
 * @param {QUERY_JOB_TYPE} extractJobType
 * @param {string} streamId
 * @param {number} logEventIdx
 * @param {Function} onUploadProgress Callback to handle upload progress events.
 * @return {Promise<axios.AxiosResponse<ExtractIrResp|ExtractJsonResp>>}
 */
const submitExtractStreamJob = async (extractJobType, streamId, logEventIdx, onUploadProgress) => {
    return await axios.post(
        "/query/extract-stream",
        {extractJobType, streamId, logEventIdx},
        {onUploadProgress}
    );
};

export {submitExtractStreamJob};
