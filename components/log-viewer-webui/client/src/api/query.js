import axios from "axios";


/**
 * @typedef {object} ExtractStreamResp
 * @property {string} stream_id
 * @property {number} begin_msg_ix
 * @property {number} end_msg_ix
 * @property {boolean} is_last_chunk
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
 * @return {Promise<axios.AxiosResponse<ExtractStreamResp>>}
 */
const submitExtractStreamJob = async (extractJobType, streamId, logEventIdx, onUploadProgress) => {
    return await axios.post(
        "/query/extract-stream",
        {extractJobType, streamId, logEventIdx},
        {onUploadProgress}
    );
};

export {submitExtractStreamJob};
