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
 * Clears current search and aggregation query results on server.
 */
const handleClearResults = () => {
    const {searchUiState, searchJobId, aggregationJobId} = useSearchStore.getState();

    // In the starting state, there are no results to clear.
    if (searchUiState === SEARCH_UI_STATE.DEFAULT) {
        return;
    }

    if (null === searchJobId) {
        console.error("Cannot clear results: searchJobId is not set.");

        return;
    }

    if (null === aggregationJobId) {
        console.error("Cannot clear results: aggregationJobId is not set.");

        return;
    }

    clearQueryResults(
        {searchJobId, aggregationJobId}
    ).catch((err: unknown) => {
        console.error("Failed to clear query results:", err);
    });
};

/**
 * Submits a new search query to server.
 *
 * @param payload
 */
const handleQuerySubmit = (payload: QueryJobCreationSchema) => {
    const store = useSearchStore.getState();

    // Buttons to submit a query should be disabled while an existing query is in progress.
    if (
        store.searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        store.searchUiState !== SEARCH_UI_STATE.DONE
    ) {
        console.error("Cannot submit query while existing query is in progress.");

        return;
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
        .catch((err: unknown) => {
            console.error("Failed to submit query:", err);
        });
};

/**
 * Cancels an ongoing search query on server.
 *
 * @param payload
 */
const handleQueryCancel = (payload: QueryJobSchema) => {
    const store = useSearchStore.getState();

    if (store.searchUiState !== SEARCH_UI_STATE.QUERYING) {
        console.error("Cannot cancel query if there is no ongoing query.");

        return;
    }

    store.updateSearchUiState(SEARCH_UI_STATE.DONE);
    cancelQuery(
        payload
    ).then(() => {
        console.log("Query cancelled successfully");
    })
        .catch((err: unknown) => {
            console.error("Failed to cancel query:", err);
        });
};


export {
    handleQueryCancel,
    handleQuerySubmit,
};
