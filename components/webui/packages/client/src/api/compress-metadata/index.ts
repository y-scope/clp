import type {components} from "@webui/api-client";

import {apiClient} from "../search";


type CompressionMetadata = components["schemas"]["CompressionMetadata"];


/**
 * Retrieves recent compression jobs (last 30 days).
 *
 * @return Recent compression jobs metadata.
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchCompressionJobs = async (): Promise<CompressionMetadata[]> => {
    // eslint-disable-next-line new-cap
    const {data, response} = await apiClient.GET("/metadata/compression_jobs", {});
    if ("undefined" === typeof data) {
        throw new Error(`Failed to fetch compression jobs: HTTP ${response.status}`);
    }

    return data;
};


export {fetchCompressionJobs};
