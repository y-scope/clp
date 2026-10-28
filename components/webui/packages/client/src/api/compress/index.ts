import type {components} from "@webui/api-client";

import {apiClient} from "../search";
import {createApiError} from "../utils";


type CompressionJob = components["schemas"]["CompressionJob"];

type CompressionJobCreation = components["schemas"]["CompressionJobCreation"];

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
    const {data, error, response} = await apiClient.POST("/compression/jobs", {body: payload});

    if ("undefined" === typeof data) {
        throw createApiError("Failed to submit compression job", response, error);
    }

    return data;
};


export type {CompressionJobCreation};
export {submitCompressionJob};
