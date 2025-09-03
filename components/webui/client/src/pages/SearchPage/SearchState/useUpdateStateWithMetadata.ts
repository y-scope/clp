import {useEffect} from "react";

import {
    PRESTO_SEARCH_SIGNAL,
    SEARCH_SIGNAL,
} from "@webui/common";
import {notification} from "antd";

import useSearchStore from "./index";
import {SEARCH_UI_STATE} from "./typings";
import {useResultsMetadata} from "./useResultsMetadata";


/*
* Presto error name for user cancellation
*/
const PRESTO_CANCEL_ERROR_NAME = "USER_CANCELED";

/**
 * Custom hook to update the client state based on results metadata from the server.
 * - Sets the UI state to `DONE` when the results metadata signal indicates that the query is
 * complete, or `FAILED` if the query fails. If there is an error, it will display a notification
 * with the error message.
 * - Updates the number of search results from the metadata.
 */
const useUpdateStateWithMetadata = () => {
    const {updateSearchUiState, updateNumSearchResultsMetadata} = useSearchStore();
    const resultsMetadata = useResultsMetadata();

    useEffect(() => {
        if (null === resultsMetadata) {
            return;
        }

        if ("undefined" !== typeof resultsMetadata.numTotalResults) {
            updateNumSearchResultsMetadata(resultsMetadata.numTotalResults);
        }

        switch (resultsMetadata.lastSignal) {
            case SEARCH_SIGNAL.RESP_DONE:
            case PRESTO_SEARCH_SIGNAL.FINISHED:
                updateSearchUiState(SEARCH_UI_STATE.DONE);
                break;
            case PRESTO_SEARCH_SIGNAL.FAILED:
                // Presto reports query cancellation as a failure, but we treat as a successful
                // completion.
                if (resultsMetadata.errorName === PRESTO_CANCEL_ERROR_NAME) {
                    updateSearchUiState(SEARCH_UI_STATE.DONE);

                    break;
                }
                updateSearchUiState(SEARCH_UI_STATE.FAILED);
                notification.error({
                    description: resultsMetadata.errorMsg || "An error occurred during search",
                    duration: 15,
                    key: `search-failed-${resultsMetadata._id}`,
                    message: resultsMetadata.errorName || "Search Failed",
                    pauseOnHover: true,
                    placement: "bottomRight",
                    showProgress: true,
                });
                break;
            default:
                break;
        }
    }, [
        resultsMetadata,
        updateSearchUiState,
        updateNumSearchResultsMetadata,
    ]);
};

export {useUpdateStateWithMetadata};
