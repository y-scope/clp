import {useEffect} from "react";

import {SEARCH_SIGNAL} from "@common/index.js";

import {useResultsMetadata} from "../reactive-mongo-queries/useResultsMetadata";
import useSearchStore from "./index";
import {SEARCH_UI_STATE} from "./typings";


/**
 * Custom hook to update the UI state when the search signal from results metadata indicates
 * that the search is done.
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
    }, [resultsMetadata,
        updateSearchUiState]);
};

export {useUiUpdateOnDoneSignal};
