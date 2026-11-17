import type {components} from "@webui/api-client";

import {apiClient} from "../search";
import {createApiError} from "../utils";


type CompressionMetadata = components["schemas"]["CompressionMetadata"];

type CompressionMetadataDisplayConfig = components["schemas"]["CompressionMetadataDisplayConfig"];

/**
 * Retrieves up to 1,000 of the most recent compression jobs.
 *
 * @return Recent compression jobs metadata.
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchCompressionJobs = async (): Promise<CompressionMetadata[]> => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/metadata/compression_jobs", {});
    if ("undefined" === typeof data) {
        throw createApiError("Failed to fetch compression jobs", response, error);
    }

    return data;
};


export type {
    CompressionMetadata,
    CompressionMetadataDisplayConfig,
};
export {fetchCompressionJobs};
