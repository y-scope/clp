import axios, {AxiosResponse} from "axios";

// eslint-disable-next-line no-warning-comments
// TODO: Replace with shared server types once the `@common` directory refactoring is completed.
type QueryJobCreationSchema = {
    ignoreCase: boolean;
    queryString: string;
    timeRangeBucketSizeMillis: number;
    timestampBegin: number;
    timestampEnd: number;
};

// eslint-disable-next-line no-warning-comments
// TODO: Replace with shared server types once the `@common` directory refactoring is completed.
type QueryJobSchema = {
    searchJobId: string;
    aggregationJobId: string;
};

const submitQuery = ({
    timestampBegin,
    timestampEnd,
    ignoreCase,
    timeRangeBucketSizeMillis,
    queryString,
}: QueryJobCreationSchema): Promise<AxiosResponse<QueryJobSchema>> => {
    const payload = {
        timestampBegin,
        timestampEnd,
        ignoreCase,
        timeRangeBucketSizeMillis,
        queryString,
    };

    console.log("Submitting query:", JSON.stringify(payload));

    return axios.post<QueryJobSchema>("/api/search/query", {data: payload});
};

export { submitQuery };
