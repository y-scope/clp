import axios, {AxiosResponse} from "axios";
import { Static } from '@sinclair/typebox'
import {
    PrestoQueryJobCreationSchema,
    PrestoQueryJobSchema,
} from "@webui/common/schemas/presto-search"

type PrestoQueryJobCreation = Static<typeof PrestoQueryJobCreationSchema>;
type PrestoQueryJob = Static<typeof PrestoQueryJobSchema>;

/**
 * Sends post request to server to submit presto query.
 *
 * @param payload
 * @return
 */
const submitQuery = async (
    payload: PrestoQueryJobCreation
): Promise<AxiosResponse<PrestoQueryJob>> => {
    console.log("Submitting query:", JSON.stringify(payload));

    return axios.post<PrestoQueryJob>("/api/presto-search/query", payload);
};


/**
 * Sends post request to server to cancel presto query.
 *
 * @param payload
 * @return
 */
const cancelQuery = async (
    payload: PrestoQueryJob
): Promise<AxiosResponse<null>> => {
    console.log("Cancelling query:", JSON.stringify(payload));

    return axios.post("/api/presto-search/cancel", payload);
};


/**
 * Sends delete request to server to clear presto query results.
 *
 * @param payload
 * @return
 */
const clearQueryResults = (payload: PrestoQueryJob): Promise<AxiosResponse<null>> => {
    console.log("Clearing query:", JSON.stringify(payload));

    return axios.delete("/api/presto-search/results", {data: payload});
};


export {
    cancelQuery,
    clearQueryResults,
    submitQuery,
};
