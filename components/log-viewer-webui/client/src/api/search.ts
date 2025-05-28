import axios, {AxiosResponse} from "axios";


// eslint-disable-next-line no-warning-comments
// TODO: Replace with shared types from the `@common` directory once refactoring is completed.
// Currently, these server types (see schemas directory) require typebox dependency so they cannot
// be moved to the `@common` directory with current implementation.
type QueryJobSchema = {
    searchJobId: string;
    aggregationJobId: string;
};

type QueryJobCreationSchema = {
    ignoreCase: boolean;
    queryString: string;
    timeRangeBucketSizeMillis: number;
    timestampBegin: number;
    timestampEnd: number;
};

/**
 * Sends query post request to server.
 *
 * @param payload
 * @return
 */
const submitQuery = (payload: QueryJobCreationSchema): Promise<AxiosResponse<QueryJobSchema>> => {
    console.log("Submitting query:", JSON.stringify(payload));

    return axios.post<QueryJobSchema>("/api/search/query", payload);
};

/**
 * Sends cancel post request to server.
 *
 * @param payload
 * @return
 */
const cancelQuery = (payload: QueryJobSchema): Promise<AxiosResponse<null>> => {
    console.log("Cancelling query:", JSON.stringify(payload));

    return axios.post("/api/search/cancel", payload);
};

/**
 * Sends query delete request to server.
 *
 * @param payload
 * @return
 */
const clearQueryResults = (payload: QueryJobSchema): Promise<AxiosResponse<null>> => {
    console.log("Clearing query:", JSON.stringify(payload));

    return axios.delete("/api/search/results", {data: payload});
};
export type {
    QueryJobCreationSchema,
    QueryJobSchema,
};
export {
    cancelQuery,
    clearQueryResults,
    submitQuery,
};
