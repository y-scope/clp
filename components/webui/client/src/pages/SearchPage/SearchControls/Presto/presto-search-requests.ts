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
    updateSearchUiState(SEARCH_UI_STATE.QUERY_ID_PENDING);


    // User should NOT be able to submit a new query while an existing query is in progress.
    if (
        searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        searchUiState !== SEARCH_UI_STATE.DONE
    ) {
        console.error("Cannot submit query while existing query is in progress.");

        return;
    }

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

            // eslint-disable-next-line no-warning-comments
            // TODO: Remove this timeout after we can show the search results in the table
            // and set ui state to DONE.
            setTimeout(() => {
                updateSearchUiState(SEARCH_UI_STATE.DONE);
            // eslint-disable-next-line no-magic-numbers
            }, 5000);
        })
        .catch((err: unknown) => {
            console.error("Failed to submit query:", err);
        });
};

/**
 * Cancels a new Presto query to server.
 *
 * @param payload
 */
const handlePrestoQueryCancel = (payload: PrestoQueryJobSchema) => {
    const {searchUiState} = useSearchStore.getState();
    if (searchUiState !== SEARCH_UI_STATE.QUERYING) {
        console.error("Cannot cancel query if there is no ongoing query.");

        return;
    }

    cancelQuery(payload)
        .then(() => {
            const {updateSearchUiState} = useSearchStore.getState();
            updateSearchUiState(SEARCH_UI_STATE.DONE);
        })
        .catch((err: unknown) => {
            console.error("Failed to cancel query:", err);
        });
};

export {
    handlePrestoQueryCancel, handlePrestoQuerySubmit,
};
