import axios, {AxiosResponse} from "axios";

import { Static } from '@sinclair/typebox'
import {
    QueryJobCreationSchema,
    QueryJobSchema,
} from "@webui/common/schemas/search"

type QueryJobCreation = Static<typeof QueryJobCreationSchema>;
type QueryJob = Static<typeof QueryJobSchema>;


/**
 * Sends post request to server to submit query.
 *
 * @param payload
 * @return
 */
const submitQuery = (payload: QueryJobCreation): Promise<AxiosResponse<QueryJobSchema>> => {
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
