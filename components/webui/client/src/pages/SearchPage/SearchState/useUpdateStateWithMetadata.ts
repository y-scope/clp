import {useEffect} from "react";

import {
    PRESTO_SEARCH_SIGNAL,
    SEARCH_SIGNAL,
} from "@webui/common/metadata";
import {notification} from "antd";

import useSearchStore from "./index";
import usePrestoSearchState from "./Presto";
import {PRESTO_SQL_INTERFACE} from "./Presto/typings";
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
    const {
        updateNumSearchResultsMetadata,
        updateSearchUiState,
    } = useSearchStore();
    const {updateErrorMsg, updateErrorName, sqlInterface} = usePrestoSearchState();
    const resultsMetadata = useResultsMetadata();

    useEffect(() => {
        if (null === resultsMetadata) {
            return;
        }

        const {searchUiState} = useSearchStore.getState();

        if (searchUiState === SEARCH_UI_STATE.DEFAULT) {
            return;
        }

        if ("undefined" !== typeof resultsMetadata.numTotalResults) {
            updateNumSearchResultsMetadata(resultsMetadata.numTotalResults);
        }

        switch (resultsMetadata.lastSignal) {
            case SEARCH_SIGNAL.RESP_DONE:
            case PRESTO_SEARCH_SIGNAL.DONE:
                updateSearchUiState(SEARCH_UI_STATE.DONE);
                break;
            case PRESTO_SEARCH_SIGNAL.FAILED: {
                const errorMsg = resultsMetadata.errorMsg || "An error occurred during search";
                const errorName = resultsMetadata.errorName || "Search Failed";

                updateErrorMsg(errorMsg);
                updateErrorName(errorName);

                // Presto reports query cancellation as a failure, but we treat as a successful
                // completion.
                if (resultsMetadata.errorName === PRESTO_CANCEL_ERROR_NAME) {
                    updateSearchUiState(SEARCH_UI_STATE.DONE);
                    break;
                }
                updateSearchUiState(SEARCH_UI_STATE.FAILED);

                // Error for guided UI is shown in drawer.
                if (sqlInterface === PRESTO_SQL_INTERFACE.FREEFORM) {
                    notification.error({
                        description: errorMsg,
                        duration: 15,
                        key: `search-failed-${resultsMetadata._id}`,
                        message: errorName,
                        pauseOnHover: true,
                        placement: "bottomRight",
                        showProgress: true,
                    });
                }
                break;
            }
            default:
                break;
        }
    }, [
        resultsMetadata,
        updateNumSearchResultsMetadata,
        updateErrorMsg,
        updateErrorName,
        updateSearchUiState,
        sqlInterface,
    ]);
};

export {useUpdateStateWithMetadata};
