import {
    createApiClient,
    type paths,
} from "@webui/api-client";


type QueryConfig = paths["/query"]["post"]["requestBody"]["content"]["application/json"];

const API_SERVER_BASE_URL = "/api/v1";

const apiClient = createApiClient({baseUrl: API_SERVER_BASE_URL});

/**
 * Submits a query job to the API server.
 *
 * @param queryConfig
 * @return The ID of the newly created query job.
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const submitQuery = async (queryConfig: QueryConfig): Promise<number> => {
    console.log("Submitting query:", JSON.stringify(queryConfig));

    // eslint-disable-next-line new-cap
    const {data, response} = await apiClient.POST("/query", {body: queryConfig});
    if ("undefined" === typeof data) {
        throw new Error(`Failed to submit query: HTTP ${response.status}`);
    }

    const jobId = Number(data.query_results_uri.split("/").pop());
    if (false === Number.isInteger(jobId)) {
        throw new Error(`Unexpected query results URI: ${data.query_results_uri}`);
    }

    return jobId;
};

/**
 * Cancels a previously submitted query job on the API server.
 *
 * @param searchJobId
 * @throws {Error} If the request fails.
 */
const cancelQuery = async (searchJobId: number): Promise<void> => {
    console.log("Cancelling query job:", searchJobId);

    // eslint-disable-next-line new-cap
    const {response} = await apiClient.DELETE("/query/{search_job_id}", {
        params: {path: {search_job_id: searchJobId}},
    });

    if (false === response.ok) {
        throw new Error(`Failed to cancel query job ${searchJobId}: HTTP ${response.status}`);
    }
};

export type {QueryConfig};
export {
    apiClient,
    cancelQuery,
    submitQuery,
};
