import axios, {AxiosResponse} from "axios";

type QueryArgs = {
    timestampBegin: number;
    timestampEnd: number;
    ignoreCase: boolean;
    timeRangeBucketSizeMillis: number;
    queryString: string;
};

type QueryResponse = {
    searchJobId: string;
    aggregationJobId: string;
};

const submitQuery = ({
    timestampBegin,
    timestampEnd,
    ignoreCase,
    timeRangeBucketSizeMillis,
    queryString,
}: QueryArgs): Promise<QueryResponse> => {
    const payload = {
        timestampBegin,
        timestampEnd,
        ignoreCase,
        timeRangeBucketSizeMillis,
        queryString,
    };

    console.log("Submitting query with payload:", payload);

    return axios.post<QueryResponse>("/api/search/query", payload)
        .then(response => response.data);
};

export { submitQuery };