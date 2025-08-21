import axios, {
    AxiosProgressEvent,
    AxiosResponse,
} from "axios";
import {Nullable} from "src/typings/common";

import {
    ExtractStreamResp,
    QUERY_JOB_TYPE,
} from "../../typings/query";


interface SubmitExtractStreamJobProps {
    dataset: Nullable<string>;
    extractJobType: QUERY_JOB_TYPE;
    streamId: string;
    logEventIdx: number;
    onUploadProgress: (progressEvent: AxiosProgressEvent) => void;
}

/**
 * Submits a job to extract the stream that contains a given log event. The stream is extracted
 * either as a CLP IR or a JSON Lines file.
 *
 * @param props
 * @param props.dataset
 * @param props.extractJobType
 * @param props.streamId
 * @param props.logEventIdx
 * @param props.onUploadProgress
 * @return The API response.
 */
const submitExtractStreamJob = async ({
    dataset,
    extractJobType,
    streamId,
    logEventIdx,
    onUploadProgress,
}: SubmitExtractStreamJobProps): Promise<AxiosResponse<ExtractStreamResp>> => {
    return await axios.post(
        "/api/stream-files/extract",
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
