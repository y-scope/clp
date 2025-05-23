import { useEffect } from "react";
import { SEARCH_SIGNAL, SearchResultsMetadataDocument } from "@common/searchResultsMetadata.js";
import { SEARCH_UI_STATE } from "../SearchState/typings";

export function useUpdateSearchUiStateOnDone(resultsMetadata: any, updateSearchUiState: (state: SEARCH_UI_STATE) => void) {
    useEffect(() => {
        if (Array.isArray(resultsMetadata) && resultsMetadata.length === 0) {
            return;
        }
        let resultMetadata = resultsMetadata[0] as SearchResultsMetadataDocument;
        if (resultMetadata.lastSignal === SEARCH_SIGNAL.RESP_DONE) {
            updateSearchUiState(SEARCH_UI_STATE.DONE);
        }
    }, [resultsMetadata, updateSearchUiState]);
}
