import axios, {AxiosResponse} from "axios";


// eslint-disable-next-line no-warning-comments
// TODO: Replace with shared type from the `@common` directory once refactoring is completed.
// Currently, server schema types require typebox dependency so they cannot be moved to the
// `@common` directory with current implementation.
type PrestoQueryJobCreationSchema = {
    queryString: string;
};

type PrestoQueryJobSchema = {
    searchJobId: string;
};


/**
 * Sends post request to server to submit presto query.
 *
 * @param payload
 * @return
 */
const submitQuery = async (
    payload: PrestoQueryJobCreationSchema
): Promise<AxiosResponse<PrestoQueryJobSchema>> => {
    console.log("Submitting query:", JSON.stringify(payload));

    return axios.post<PrestoQueryJobSchema>("/api/presto-search/query", payload);
};

export type {
    PrestoQueryJobCreationSchema,
    PrestoQueryJobSchema,
};

export {submitQuery};
