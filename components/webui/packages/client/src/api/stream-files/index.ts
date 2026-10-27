import {Nullable} from "@webui/common/utility-types";

import {ExtractStreamResp} from "../../typings/query";
import {apiClient} from "../search";
import {createApiError} from "../utils";


interface SubmitExtractStreamJobProps {
    archiveId: string;
    dataset: Nullable<string>;
    logEventIdx: number;
    onUploadProgress: () => void;
}

/**
 * Submits a job to extract the JSON Lines stream of the given archive that contains a given log
 * event.
 *
 * @param props
 * @param props.archiveId
 * @param props.dataset
 * @param props.logEventIdx
 * @param props.onUploadProgress
 * @return The API response.
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const submitExtractStreamJob = async ({
    archiveId,
    dataset,
    logEventIdx,
    onUploadProgress,
}: SubmitExtractStreamJobProps): Promise<{data: ExtractStreamResp}> => {
    onUploadProgress();

    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.POST("/stream_files/extract", {
        body: {
            archive_id: archiveId,
            dataset: dataset,
            log_event_idx: logEventIdx,
        },
    });

    if ("undefined" === typeof data) {
        throw createApiError("Failed to extract stream file", response, error);
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
