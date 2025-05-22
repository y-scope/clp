
import axios from "axios";

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

const submitQuery = async ({
    timestampBegin,
    timestampEnd,
    ignoreCase,
    timeRangeBucketSizeMillis,
    queryString,
}: QueryArgs) => {
    const payload = {
        timestampBegin,
        timestampEnd,
        ignoreCase,
        timeRangeBucketSizeMillis,
        queryString,
    };

    console.log("Submitting query with payload:", payload);

    try {
        const response =
            await axios.post<QueryResponse>("/api/search/query", payload);
        const { searchJobId, aggregationJobId } = response.data;
        return { searchJobId, aggregationJobId };
    } catch (error) {
        if (axios.isAxiosError(error)) {
            console.error("Axios error during query submission:", error.message, error.response?.data);
        } else {
            console.error("Unknown error during query submission:", error);
        }
        throw error;
    }
};

export { submitQuery };