import {
    type PrestoQueryJob,
    type PrestoQueryJobCreation,
} from "@webui/common/schemas/presto-search";

import {
    cancelQuery,
    clearQueryResults,
    submitQuery,
} from "../../../../api/presto-search";
import useSearchStore from "../../SearchState";
import {SEARCH_UI_STATE} from "../../SearchState/typings";


/**
 * Clears current presto query results on server.
 */
const handlePrestoClearResults = () => {
    const {searchUiState, searchJobId} = useSearchStore.getState();

    // In the starting state, there are no results to clear.
    if (searchUiState === SEARCH_UI_STATE.DEFAULT) {
        return;
    }

    if (null === searchJobId) {
        console.error("Cannot clear results: searchJobId is not set.");

        return;
    }

    clearQueryResults(
        {searchJobId}
    ).catch((err: unknown) => {
        console.error("Failed to clear query results:", err);
    });
};


/**
 * Submits a new Presto query to server.
 *
 * @param payload
 */
const handlePrestoQuerySubmit = (payload: PrestoQueryJobCreation) => {
    const {updateSearchJobId, updateSearchUiState, searchUiState} = useSearchStore.getState();

    // User should NOT be able to submit a new query while an existing query is in progress.
    if (
        searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        searchUiState !== SEARCH_UI_STATE.DONE &&
        searchUiState !== SEARCH_UI_STATE.FAILED
    ) {
        console.error("Cannot submit query while existing query is in progress.");

        return;
    }

    handlePrestoClearResults();

    updateSearchUiState(SEARCH_UI_STATE.QUERY_ID_PENDING);

    submitQuery(payload)
        .then((result) => {
            const {searchJobId} = result.data;
            updateSearchJobId(searchJobId);
            updateSearchUiState(SEARCH_UI_STATE.QUERYING);
            console.debug(
                "Presto search job created - ",
                "Search job ID:",
                searchJobId
            );
        })
        .catch((err: unknown) => {
            console.error("Failed to submit query:", err);
        });
};

/**
 * Cancels an ongoing Presto search query on server.
 *
 * @param payload
 */
const handlePrestoQueryCancel = (payload: PrestoQueryJob) => {
    const {searchUiState, updateSearchUiState} = useSearchStore.getState();
    if (searchUiState !== SEARCH_UI_STATE.QUERYING) {
        console.error("Cannot cancel query if there is no ongoing query.");

        return;
    }

    updateSearchUiState(SEARCH_UI_STATE.DONE);
    cancelQuery(payload)
        .then(() => {
            console.debug("Query cancelled successfully");
        })
        .catch((err: unknown) => {
            console.error("Failed to cancel query:", err);
        });
};

export {
    handlePrestoQueryCancel,
    handlePrestoQuerySubmit,
};
