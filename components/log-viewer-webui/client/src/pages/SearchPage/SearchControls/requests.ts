import {
    cancelQuery,
    clearQueryResults,
    QueryJobCreationSchema,
    QueryJobSchema,
    submitQuery,
} from "../../../api/search";
import useSearchStore from "../SearchState/index";
import {SEARCH_UI_STATE} from "../SearchState/typings";


/**
 * Clears the current search and aggregation query results.
 */
const handleClearResults = () => {
    const {searchUiState, searchJobId, aggregationJobId} = useSearchStore.getState();

    if (searchUiState === SEARCH_UI_STATE.DEFAULT) {
        return;
    }

    if (null === searchJobId || null === aggregationJobId) {
        throw new Error("No searchJobId or aggregationJobId to clear.");
    }

    clearQueryResults(
        {searchJobId, aggregationJobId}
    ).catch((error) => {
        console.error("Failed to clear query results:", error);
    });
};

/**
 * Submits a new search query.
 *
 * @param payload
 */
const handleQuerySubmit = (payload: QueryJobCreationSchema) => {
    const store = useSearchStore.getState();

    if (
        store.searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        store.searchUiState !== SEARCH_UI_STATE.DONE
    ) {
        throw new Error("Cannot submit query when not in DEFAULT or DONE state.");
    }

    handleClearResults();

    store.updateSearchUiState(SEARCH_UI_STATE.QUERY_ID_PENDING);

    submitQuery(payload)
        .then((result) => {
            store.updateSearchJobId(result.data.searchJobId);
            store.updateAggregationJobId(result.data.aggregationJobId);
            store.updateSearchUiState(SEARCH_UI_STATE.QUERYING);
            console.log("Query ID Returned", result);
        })
        .catch((error) => {
            console.error("Failed to submit query:", error);
        });
};

/**
 * Cancels an ongoing search query.
 *
 * @param payload
 */
const handleQueryCancel = (payload: QueryJobSchema) => {
    const store = useSearchStore.getState();

    if (store.searchUiState !== SEARCH_UI_STATE.QUERYING) {
        throw new Error("Cannot cancel query when not in QUERYING state.");
    }

    cancelQuery(
        payload
    ).then(() => {
        console.log("Query cancelled successfully");
    })
        .catch((error) => {
            console.error("Failed to cancel query:", error);
        });

    store.updateSearchUiState(SEARCH_UI_STATE.DONE);
};


export {
    handleQueryCancel,
    handleQuerySubmit,
};
