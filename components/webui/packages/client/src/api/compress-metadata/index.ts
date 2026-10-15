import {CompressionMetadataDecoded} from "@webui/common/schemas/compress-metadata";

import {apiClient} from "../search";
import {createApiError} from "../utils";


/**
 * Retrieves recent compression jobs (last 30 days).
 *
 * @return Recent compression jobs metadata.
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchCompressionJobs = async (): Promise<CompressionMetadataDecoded[]> => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/metadata/compression_jobs", {});
    if ("undefined" === typeof data) {
        throw createApiError("Failed to fetch compression jobs", response, error);
    }

    return data as unknown as CompressionMetadataDecoded[];
};


export {fetchCompressionJobs};
