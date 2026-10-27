import type {paths} from "@webui/api-client";
import {
    CompressionJob,
    CompressionJobCreation,
} from "@webui/common/schemas/compression";

import {apiClient} from "../search";
import {createApiError} from "../utils";


type CompressionJobCreationBody =
    paths["/compression/jobs"]["post"]["requestBody"]["content"]["application/json"];

/**
 * Submits a compression job.
 *
 * @param payload
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const submitCompressionJob = async (payload: CompressionJobCreation): Promise<CompressionJob> => {
    console.log("Submitting compression job:", JSON.stringify(payload));

    const body: CompressionJobCreationBody = {paths: payload.paths};
    if ("undefined" !== typeof payload.dataset) {
        body.dataset = payload.dataset;
    }
    if ("undefined" !== typeof payload.timestampKey && 0 < payload.timestampKey.length) {
        body.timestamp_key = payload.timestampKey;
    }
    if ("undefined" !== typeof payload.unstructured) {
        body.unstructured = payload.unstructured;
    }

    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.POST("/compression/jobs", {body});

    if ("undefined" === typeof data) {
        throw createApiError("Failed to submit compression job", response, error);
    }

    return {jobId: data.job_id};
};


export {submitCompressionJob};
