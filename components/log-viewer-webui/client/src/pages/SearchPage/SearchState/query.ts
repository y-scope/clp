import useSearchStore from "./index";
import { clearQueryResults } from "../../../api/search/queryClear";
import { submitQuery } from "../../../api/search/querySubmit";
import { cancelQuery } from "../../../api/search/queryCancel";
import { SEARCH_UI_STATE } from "./typings";

const handleClearResults = () => {
    const store = useSearchStore.getState();

    if (store.searchUiState === SEARCH_UI_STATE.DEFAULT) {
        return;
    }

    const searchJobId = store.searchJobId;
    const aggregationJobId = store.aggregationJobId;

    if (!searchJobId || !aggregationJobId) {
        throw new Error("No searchJobId or aggregationJobId to clear.");
    }

    clearQueryResults({
        searchJobId,
        aggregationJobId,
    }).catch((error) => {
        console.error("Failed to clear query results:", error);
    });

};

// Add handleQuerySubmit that uses submitQuery from the API
type QueryArgs = {
    timestampBegin: number;
    timestampEnd: number;
    ignoreCase: boolean;
    timeRangeBucketSizeMillis: number;
    queryString: string;
};

const handleQuerySubmit = () => {

    const store = useSearchStore.getState();

    if (store.searchUiState === SEARCH_UI_STATE.QUERY_SUBMITTED ) {
        console.warn("Query already submitted, ignoring submit request.");
        return;
    }

    if (store.searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        store.searchUiState !== SEARCH_UI_STATE.DONE
    ) {
        throw new Error("Cannot submit query when not in DEFAULT or DONE state.");
    }

    handleClearResults();

    const args: QueryArgs = {
        queryString: store.queryString,
        timestampBegin: store.timeRange[0].valueOf(),
        timestampEnd: store.timeRange[1].valueOf(),
        ignoreCase: false,
        timeRangeBucketSizeMillis: 150,
    };

    store.updateSearchUiState(SEARCH_UI_STATE.QUERY_SUBMITTED);

    submitQuery(args)
        .then((result) => {
            store.updateSearchJobId(result.searchJobId);
            store.updateAggregationJobId(result.aggregationJobId);
            store.updateSearchUiState(SEARCH_UI_STATE.QUERYING);
        })
        .catch((error) => {
            console.error("Failed to submit query:", error);
        });
};

const handleQueryCancel = () => {
    const store = useSearchStore.getState();

    if (store.searchUiState !== SEARCH_UI_STATE.QUERYING) {
       throw new Error("Cannot cancel query when not in QUERYING state.");
    }

    const searchJobId = store.searchJobId;
    const aggregationJobId = store.aggregationJobId;

    if (!searchJobId || !aggregationJobId) {
        throw new Error("Cannot cancel query without jobIDs");
    }

    cancelQuery({
        searchJobId,
        aggregationJobId,
    }).then(() => {
        console.log("Query cancelled successfully");
    }).catch((error) => {
        console.error("Failed to cancel query:", error);
    });

    // Reset the state. I dont need to wait for promise to resolve. Another query
    // can be submitted before previous cancel finished
    store.updateSearchJobId(null);
    store.updateAggregationJobId(null);
    store.updateSearchUiState(SEARCH_UI_STATE.DEFAULT);
};

export {
    handleClearResults,
    handleQuerySubmit,
    handleQueryCancel,
};