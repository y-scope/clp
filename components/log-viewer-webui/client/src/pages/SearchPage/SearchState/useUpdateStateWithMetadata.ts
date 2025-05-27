import { useEffect } from "react";
import { SEARCH_UI_STATE } from "./typings";
import { SEARCH_SIGNAL } from "@common/index.js";
import useSearchStore  from "./index";
import { useResultsMetadata } from "../reactive-mongo-queries/useResultsMetadata";

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
    }, [resultsMetadata]);
}

export { useUiUpdateOnDoneSignal };
