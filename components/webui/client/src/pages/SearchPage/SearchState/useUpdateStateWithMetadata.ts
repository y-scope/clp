import {useEffect} from "react";

import {
    PRESTO_SEARCH_SIGNAL,
    SEARCH_SIGNAL,
} from "@common/index.js";
import {notification} from "antd";

import useSearchStore from "./index";
import {SEARCH_UI_STATE} from "./typings";
import {useResultsMetadata} from "./useResultsMetadata";


/**
 * Custom hook to update the UI state to `DONE` when the results metadata signal indicates
 * that the query is complete, or `FAILURE` if the query fails. If there is an error, it will
 * also display a notification with the error message.
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
                description: resultsMetadata.errorMsg || "An error occurred during search",
                duration: 15,
                key: `search-failed-${resultsMetadata._id}`,
                message: resultsMetadata.errorName || "Search Failed",
                pauseOnHover: true,
                placement: "bottomRight",
                showProgress: true,
            });
        }
    }, [
        resultsMetadata,
        updateSearchUiState,
    ]);
};

export {useUiUpdateOnDoneSignal};
