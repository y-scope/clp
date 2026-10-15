import {
    CompressionJob,
    CompressionJobCreation,
} from "@webui/common/schemas/compression";

import {apiClient} from "../search";
import {createApiError} from "../utils";


/**
 * Submits a compression job.
 *
 * @param payload
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const submitCompressionJob = async (payload: CompressionJobCreation): Promise<CompressionJob> => {
    console.log("Submitting compression job:", JSON.stringify(payload));

    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.POST("/compression/jobs", {
        body: {
            dataset: payload.dataset ?? null,
            paths: payload.paths,
            timestamp_key: payload.timestampKey ?? null,
            unstructured: payload.unstructured ?? null,
        },
    });

    if ("undefined" === typeof data) {
        throw createApiError("Failed to submit compression job", response, error);
    }

    return {jobId: data.job_id};
};


export {submitCompressionJob};
