import {
    type QueryJob,
    type QueryJobCreation,
} from "@webui/common/schemas/search";
import axios, {AxiosResponse} from "axios";


/**
 * Sends post request to server to submit query.
 *
 * @param payload
 * @return
 */
const submitQuery = (payload: QueryJobCreation): Promise<AxiosResponse<QueryJob>> => {
    console.log("Submitting query:", JSON.stringify(payload));

    return axios.post<QueryJob>("/api/search/query", payload);
};

/**
 * Sends post request to server to cancel query.
 *
 * @param payload
 * @return
 */
const cancelQuery = (payload: QueryJob): Promise<AxiosResponse<null>> => {
    console.log("Cancelling query:", JSON.stringify(payload));

    return axios.post("/api/search/cancel", payload);
};

/**
 * Sends delete request to server to clear query results.
 *
 * @param payload
 * @return
 */
const clearQueryResults = (payload: QueryJob): Promise<AxiosResponse<null>> => {
    console.log("Clearing query:", JSON.stringify(payload));

    return axios.delete("/api/search/results", {data: payload});
};

export {
    cancelQuery,
    clearQueryResults,
    submitQuery,
};
