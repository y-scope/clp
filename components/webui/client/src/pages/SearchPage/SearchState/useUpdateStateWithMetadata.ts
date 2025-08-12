import {useEffect} from "react";
import {notification} from "antd";

import {SEARCH_SIGNAL, PRESTO_SEARCH_SIGNAL} from "@common/index.js";

import useSearchStore from "./index";
import {SEARCH_UI_STATE} from "./typings";
import {useResultsMetadata} from "./useResultsMetadata";


/**
 * Custom hook to update the UI state to `DONE` when the results metadata signal indicates
 * that the query is complete, or `FAILURE` if the query fails.
 */
const useUiUpdateOnDoneSignal = () => {
    const {updateSearchUiState} = useSearchStore();
    const resultsMetadata = useResultsMetadata();

    useEffect(() => {
        if (null === resultsMetadata) {
            return;
        }

        if (resultsMetadata.lastSignal === SEARCH_SIGNAL.RESP_DONE) {
            updateSearchUiState(SEARCH_UI_STATE.DONE);
        } else if (resultsMetadata.lastSignal === PRESTO_SEARCH_SIGNAL.FAILED) {
            updateSearchUiState(SEARCH_UI_STATE.FAILED);
            notification.error({
                key: `search-failed-${resultsMetadata._id}`,
                message: resultsMetadata.errorType || "Search Failed",
                description: resultsMetadata.errorMsg || "An error occurred during search",
                duration: null,
                placement: "bottomRight",
            });
        }
    }, [
        resultsMetadata,
        updateSearchUiState,
    ]);
};

export {useUiUpdateOnDoneSignal};
