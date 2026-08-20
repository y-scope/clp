import {apiClient} from "../../../../../api/search";


interface QuerySpeedResp {
    bytes: number | null;
    duration: number | null;
}

/**
 * Fetches the query speed data (bytes and duration) for a specific job ID
 * across the given datasets.
 *
 * @param datasetNames
 * @param jobId
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchQuerySpeed = async (datasetNames: string[], jobId: string): Promise<QuerySpeedResp> => {
    // eslint-disable-next-line new-cap
    const {data, response} = await apiClient.GET("/metadata/query_speed", {
        params: {
            query: {
                dataset: datasetNames.join(","),
                search_job_id: Number(jobId),
            },
        },
    });

    if ("undefined" === typeof data) {
        throw new Error(`Failed to fetch query speed: HTTP ${response.status}`);
    }

    return {
        bytes: data.bytes ?? null,
        duration: data.duration ?? null,
    };
};

export {fetchQuerySpeed};
