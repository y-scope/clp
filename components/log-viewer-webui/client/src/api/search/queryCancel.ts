import axios from "axios";

type QueryCancelArgs = {
    searchJobId: string;
    aggregationJobId: string;
};

const cancelQuery = async ({
    searchJobId,
    aggregationJobId,
}: QueryCancelArgs) => {
    const payload = {
        searchJobId,
        aggregationJobId,
    };

    console.log("Cancelling query with payload:", payload);

    try {
        await axios.post("/api/search/cancel", payload);
    } catch (error) {
        if (axios.isAxiosError(error)) {
            console.error("Axios error during query cancel:", error.message, error.response?.data);
        } else {
            console.error("Unknown error during query cancel:", error);
        }
        throw error;
    }
};

export { cancelQuery };
