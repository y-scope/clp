import {apiClient} from "../search";
import {createApiError} from "../utils";
import {parseCompressionJobs} from "./parse";


/**
 * Retrieves up to 1,000 of the most recent compression jobs.
 *
 * @return Recent compression jobs metadata.
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchCompressionJobs = async () => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/metadata/compression_jobs", {});
    if ("undefined" === typeof data) {
        throw createApiError("Failed to fetch compression jobs", response, error);
    }

    return parseCompressionJobs(data);
};


export {fetchCompressionJobs};
