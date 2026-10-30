import type {components} from "@webui/api-client";
import {Nullable} from "@webui/common/utility-types";

import {apiClient} from "../search";
import {createApiError} from "../utils";


type StreamFileMetadata = components["schemas"]["StreamFileMetadata"];

interface SubmitExtractStreamJobProps {
    archiveId: string;
    dataset: Nullable<string>;
    logEventIdx: number;
}

/**
 * Submits a job to extract the JSON Lines stream of the given archive that contains a given log
 * event.
 *
 * @param props
 * @param props.archiveId
 * @param props.dataset
 * @param props.logEventIdx
 * @return The metadata of the extracted stream file.
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const submitExtractStreamJob = async ({
    archiveId,
    dataset,
    logEventIdx,
}: SubmitExtractStreamJobProps): Promise<StreamFileMetadata> => {
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

    return data;
};

export {submitExtractStreamJob};
