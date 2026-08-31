import {QUERY_JOB_TYPE} from "@webui/common/query";
import {Nullable} from "@webui/common/utility-types";

import {ExtractStreamResp} from "../../typings/query";
import {apiClient} from "../search";


interface SubmitExtractStreamJobProps {
    dataset: Nullable<string>;
    extractJobType: QUERY_JOB_TYPE;
    streamId: string;
    logEventIdx: number;
    onUploadProgress: () => void;
}

/**
 * Maps the numeric `QUERY_JOB_TYPE` enum to the string enum expected by the api-server's
 * `ExtractJobType` schema.
 *
 * @param jobType
 * @return
 */
const mapExtractJobType = (jobType: QUERY_JOB_TYPE): "ExtractIr" | "ExtractJson" => {
    if (QUERY_JOB_TYPE.EXTRACT_IR === jobType) {
        return "ExtractIr";
    }

    return "ExtractJson";
};

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
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const submitExtractStreamJob = async ({
    dataset,
    extractJobType,
    streamId,
    logEventIdx,
    onUploadProgress,
}: SubmitExtractStreamJobProps): Promise<{data: ExtractStreamResp}> => {
    onUploadProgress();

    // eslint-disable-next-line new-cap
    const {data, response} = await apiClient.POST("/stream_files/extract", {
        body: {
            dataset: dataset,
            extract_job_type: mapExtractJobType(extractJobType),
            log_event_idx: logEventIdx,
            stream_id: streamId,
        },
    });

    if ("undefined" === typeof data) {
        throw new Error(`Failed to extract stream file: HTTP ${response.status}`);
    }

    return {
        data: {
            _id: data.stream_id,
            begin_msg_ix: data.begin_msg_ix,
            end_msg_ix: data.end_msg_ix,
            is_last_chunk: data.is_last_chunk,
            path: data.path,
            stream_id: data.stream_id,
        },
    };
};

export {submitExtractStreamJob};
