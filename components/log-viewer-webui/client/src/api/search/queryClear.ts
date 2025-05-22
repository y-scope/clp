import axios from "axios";

type QueryClearArgs = {
    searchJobId: string;
    aggregationJobId: string;
};

const clearQueryResults = async ({
    searchJobId,
    aggregationJobId,
}: QueryClearArgs) => {
    const payload = {
        searchJobId,
        aggregationJobId,
    };

    console.log("Clearing query results with payload:", payload);

    try {
        await axios.delete("/api/search/results", { data: payload });
    } catch (error) {
        if (axios.isAxiosError(error)) {
            console.error("Axios error during query clear:", error.message, error.response?.data);
        } else {
            console.error("Unknown error during query clear:", error);
        }
        throw error;
    }
};

export { clearQueryResults };
