import {useEffect} from "react";

import {SEARCH_SIGNAL} from "@common/index.js";

import {useResultsMetadata} from "../reactive-mongo-queries/useResultsMetadata";
import useSearchStore from "./index";
import {SEARCH_UI_STATE} from "./typings";


/**
 * Custom hook to update the UI state to `DONE` when the results metadata signal indicates
 * that the query is complete.
 */
const useUiUpdateOnDoneSignal = () => {
    const {updateSearchUiState} = useSearchStore();
    const resultsMetadata = useResultsMetadata();
    useEffect(() => {
        if (null === resultsMetadata
        ) {
            return;
        }

        if (resultsMetadata.lastSignal === SEARCH_SIGNAL.RESP_DONE) {
            updateSearchUiState(SEARCH_UI_STATE.DONE);
        }
    }, [
        resultsMetadata,
        updateSearchUiState,
    ]);
};

export {useUiUpdateOnDoneSignal};
