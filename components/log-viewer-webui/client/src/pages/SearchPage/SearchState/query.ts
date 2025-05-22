import useSearchStore from "./index";
import { clearQueryResults } from "../../../api/search/queryClear";
import { submitQuery } from "../../../api/search/querySubmit";
import { cancelQuery } from "../../../api/search/queryCancel";

export const handleClearResults = async () => {
    const store = useSearchStore.getState();

    const searchJobId = store.searchJobId;
    const aggregationJobId = store.aggregationJobId;

    if (!searchJobId || !aggregationJobId) {
        console.warn("No searchJobId or aggregationJobId to clear.");
        return;
    }

    try {
        await clearQueryResults({
            searchJobId,
            aggregationJobId,
        });
        store.updateSearchJobId(null);
        store.updateAggregationJobId(null);
        console.log("Query results cleared successfully");
    } catch (error) {
        console.error("Failed to clear query results:", error);
    }
};

// Add handleQuerySubmit that uses submitQuery from the API
type QueryArgs = {
    timestampBegin: number;
    timestampEnd: number;
    ignoreCase: boolean;
    timeRangeBucketSizeMillis: number;
    queryString: string;
};

export const handleQuerySubmit = async (args: QueryArgs) => {
    const store = useSearchStore.getState();

    const searchJobId = store.searchJobId;

    if (null !== searchJobId) {
        // Clear result caches before starting a new query
        await handleClearResults();
    }

    const result = await submitQuery(args);

    // Update store with new job IDs
    store.updateSearchJobId(result.searchJobId);
    store.updateAggregationJobId(result.aggregationJobId);

    return result;
};

export const handleQueryCancel = async () => {
    const store = useSearchStore.getState();

    const searchJobId = store.searchJobId;
    const aggregationJobId = store.aggregationJobId;

    if (!searchJobId || !aggregationJobId) {
        console.warn("No searchJobId or aggregationJobId to cancel.");
        return;
    }

    try {
        await cancelQuery({
            searchJobId,
            aggregationJobId,
        });
        store.updateSearchJobId(null);
        store.updateAggregationJobId(null);
        console.log("Query cancelled successfully");
    } catch (error) {
        console.error("Failed to cancel query:", error);
    }
};