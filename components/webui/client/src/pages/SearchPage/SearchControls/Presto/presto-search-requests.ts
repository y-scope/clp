import {
    cancelQuery,
    type PrestoQueryJobCreationSchema,
    type PrestoQueryJobSchema,
    submitQuery,
} from "../../../../api/presto-search";
import useSearchStore from "../../SearchState";
import {SEARCH_UI_STATE} from "../../SearchState/typings";


/**
 * Submits a new Presto query to server.
 *
 * @param payload
 */
const handlePrestoQuerySubmit = (payload: PrestoQueryJobCreationSchema) => {
    const {updateSearchJobId, updateSearchUiState, searchUiState} = useSearchStore.getState();

    // User should NOT be able to submit a new query while an existing query is in progress.
    if (
        searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        searchUiState !== SEARCH_UI_STATE.DONE
    ) {
        console.error("Cannot submit query while existing query is in progress.");

        return;
    }

    updateSearchUiState(SEARCH_UI_STATE.QUERY_ID_PENDING);

    submitQuery(payload)
        .then((result) => {
            const {searchJobId} = result.data;

            updateSearchJobId(searchJobId);
            updateSearchUiState(SEARCH_UI_STATE.QUERYING);

            // eslint-disable-next-line no-warning-comments
            // TODO: Delete previous query results when the backend is ready

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
const handlePrestoQueryCancel = (payload: PrestoQueryJobSchema) => {
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
    handlePrestoQueryCancel, handlePrestoQuerySubmit,
};
