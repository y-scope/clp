import {
    type components,
    createApiClient,
    type paths,
} from "@webui/api-client";


type Aggregation = components["schemas"]["Aggregation"];

type QueryConfig =
    paths["/query/results_cache"]["post"]["requestBody"]["content"]["application/json"];

const API_SERVER_BASE_URL = "/api/v1";

const apiClient = createApiClient({baseUrl: API_SERVER_BASE_URL});

/**
 * Submits a query job whose results are buffered in the results cache to the API server.
 *
 * @param queryConfig
 * @return The ID of the newly created query job.
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const submitQuery = async (queryConfig: QueryConfig): Promise<number> => {
    console.log("Submitting query:", JSON.stringify(queryConfig));

    // Buffer results in the results cache (MongoDB) so that the results contain structured metadata
    // (e.g., timestamps and original file paths). Also, the `clp` storage engine doesn't support
    // writing search results to files.
    // eslint-disable-next-line new-cap
    const {data, response} = await apiClient.POST("/query/results_cache", {body: queryConfig});
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
 * @param queryJobId
 * @throws {Error} If the request fails.
 */
const cancelQuery = async (queryJobId: number): Promise<void> => {
    console.log("Cancelling query job:", queryJobId);

    // eslint-disable-next-line new-cap
    const {response} = await apiClient.DELETE("/query/{query_job_id}", {
        params: {path: {query_job_id: queryJobId}},
    });

    if (false === response.ok) {
        throw new Error(`Failed to cancel query job ${queryJobId}: HTTP ${response.status}`);
    }
};

export type {
    Aggregation,
    QueryConfig,
};
export {
    apiClient,
    cancelQuery,
    submitQuery,
};
