import { useEffect } from "react";
import { SEARCH_UI_STATE } from "./typings";
import { SEARCH_SIGNAL, SearchResultsMetadataDocument } from "@common/index.js";
import useSearchStore  from "./index";
import { Nullable } from "../../../typings/common";


const useUpdateUiStateWithMetadata = (resultsMetadata: Nullable<SearchResultsMetadataDocument[]>) => {
    const {updateSearchUiState} = useSearchStore();
    useEffect(() => {
        if (null === resultsMetadata ||
            (Array.isArray(resultsMetadata) && resultsMetadata.length === 0)
        ) {
            return;
        }

        let resultMetadata = resultsMetadata[0] as SearchResultsMetadataDocument;
        if (resultMetadata.lastSignal === SEARCH_SIGNAL.RESP_DONE) {
            updateSearchUiState(SEARCH_UI_STATE.DONE);
        }
    }, [resultsMetadata]);
}

export { useUpdateUiStateWithMetadata} ;