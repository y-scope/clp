import {message} from "antd";

import {
    cancelQuery,
    clearQueryResults,
    QueryJobCreationSchema,
    QueryJobSchema,
    submitQuery,
} from "../../../api/search";
import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../SearchState/";
import {SEARCH_UI_STATE} from "../SearchState/typings";
import {unquoteString} from "./utils";


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

    // User should NOT be able to submit a new query while an existing query is in progress.
    if (
        store.searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        store.searchUiState !== SEARCH_UI_STATE.DONE
    ) {
        console.error("Cannot submit query while existing query is in progress.");

        return;
    }

    if (CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE) {
        try {
            payload.queryString = unquoteString(payload.queryString, '"', "\\");
            if ("" === payload.queryString) {
                message.error("Query string cannot be empty.");

                return;
            }
        } catch (e: unknown) {
            message.error(`Error processing query string: ${e instanceof Error ?
                e.message :
                String(e)}`);

            return;
        }
    }

    handleClearResults();

    store.updateNumSearchResultsTable(SEARCH_STATE_DEFAULT.numSearchResultsTable);
    store.updateNumSearchResultsTimeline(SEARCH_STATE_DEFAULT.numSearchResultsTimeline);
    store.updateSearchUiState(SEARCH_UI_STATE.QUERY_ID_PENDING);

    submitQuery(payload)
        .then((result) => {
            const {searchJobId, aggregationJobId} = result.data;
            store.updateSearchJobId(searchJobId);
            store.updateAggregationJobId(aggregationJobId);
            store.updateSearchUiState(SEARCH_UI_STATE.QUERYING);
            console.debug(
                "Search job created - ",
                "Search job ID:",
                searchJobId,
                "Aggregation job ID:",
                aggregationJobId
            );
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
        console.debug("Query cancelled successfully");
    })
        .catch((err: unknown) => {
            console.error("Failed to cancel query:", err);
        });
};


export {
    handleQueryCancel,
    handleQuerySubmit,
};
