import {apiClient} from "../../../../../api/search";
import {createApiError} from "../../../../../api/utils";


interface QuerySpeedResp {
    bytes: number | null;
    duration: number | null;
}

/**
 * Fetches the query speed data (bytes and duration) for a specific job ID
 * across the given datasets. Both fields are `null` until the job has scanned
 * archives and finished.
 *
 * @param datasetNames
 * @param jobId
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchQuerySpeed = async (datasetNames: string[], jobId: string): Promise<QuerySpeedResp> => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/metadata/query_speed", {
        params: {
            query: {
                datasets: datasetNames.join(","),
                query_job_id: Number(jobId),
            },
        },
    });

    if ("undefined" === typeof data) {
        throw createApiError("Failed to fetch query speed", response, error);
    }

    return {
        bytes: data?.bytes ?? null,
        duration: data?.duration ?? null,
    };
};

export {fetchQuerySpeed};
