import axios, {
    AxiosProgressEvent,
    AxiosResponse,
} from "axios";

import {
    ExtractStreamResp,
    QUERY_JOB_TYPE,
} from "../typings/query";


interface ExtractStreamJobOptions {
    dataset: string;
    extractJobType: QUERY_JOB_TYPE;
    streamId: string;
    logEventIdx: number;
    onUploadProgress: (progressEvent: AxiosProgressEvent) => void;
}

/**
 * Submits a job to extract the stream that contains a given log event. The stream is extracted
 * either as a CLP IR or a JSON Lines file.
 *
 * @param options The extraction job options.
 * @return The API response.
 */
const submitExtractStreamJob = async (
    options: ExtractStreamJobOptions
): Promise<AxiosResponse<ExtractStreamResp>> => {
    const {dataset, extractJobType, streamId, logEventIdx, onUploadProgress} = options;

    return await axios.post(
        "/query/extract-stream",
        {
            dataset,
            extractJobType,
            streamId,
            logEventIdx,
        },
        {onUploadProgress}
    );
};

export {submitExtractStreamJob};
export type {ExtractStreamJobOptions};
