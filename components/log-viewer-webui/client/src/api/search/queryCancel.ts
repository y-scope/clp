import axios from "axios";

type QueryCancelArgs = {
    searchJobId: string;
    aggregationJobId: string;
};

const cancelQuery = ({
    searchJobId,
    aggregationJobId,
}: QueryCancelArgs): Promise<import("axios").AxiosResponse<any>> => {
    const payload = {
        searchJobId,
        aggregationJobId,
    };

    console.log("Cancelling query with payload:", payload);

    return axios.post("/api/search/cancel", payload);
};

export { cancelQuery };
