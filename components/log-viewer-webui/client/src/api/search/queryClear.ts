import axios from "axios";

type QueryClearArgs = {
    searchJobId: string;
    aggregationJobId: string;
};

const clearQueryResults = ({
    searchJobId,
    aggregationJobId,
}: QueryClearArgs): Promise<import("axios").AxiosResponse<any>> => {
    const payload = {
        searchJobId,
        aggregationJobId,
    };

    console.log("Clearing query results with payload:", payload);

    return axios.delete("/api/search/results", { data: payload });
};

export { clearQueryResults };
