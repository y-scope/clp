import axios, {AxiosResponse} from "axios";

// eslint-disable-next-line no-warning-comments
// TODO: Replace with shared server types once the `@common` directory refactoring is completed.
type QueryJobSchema = {
    searchJobId: string;
    aggregationJobId: string;
};

const cancelQuery = ({
    searchJobId,
    aggregationJobId,
}: QueryJobSchema): Promise<AxiosResponse<null>> => {
    const payload = {
        searchJobId,
        aggregationJobId,
    };

    console.log("Cancelling query:", JSON.stringify(payload));
    return axios.post("/api/search/cancel", payload);
};

export { cancelQuery };
