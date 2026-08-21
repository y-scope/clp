import {useQueryResults} from "@webui/api-client/useQueryResults";

import {apiClient} from "../../../../../../api/search";
import useSearchStore from "../../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {SearchResult} from "./typings";


/**
 * Custom hook to stream search results for the current searchJobId from the API server's SSE
 * endpoint. The search UI state is updated to `DONE` only after this stream and the aggregation
 * stream have both ended.
 *
 * @return
 */
const useSearchResults = () => {
    const searchJobId = useSearchStore((state) => state.searchJobId);

    return useQueryResults<SearchResult>(apiClient, searchJobId, {
        // Sort by timestamp (desc) to match the previous MongoDB cursor ordering.
        compareFn: (a, b) => b.timestamp - a.timestamp,
        onDone: () => {
            if (null !== searchJobId) {
                useSearchStore.getState().markSearchResultsComplete(searchJobId);
            }
        },
        onError: (err) => {
            console.error("Failed to stream search results:", err);
            const {searchUiState, updateSearchUiState, searchJobId: currentJobId} =
                useSearchStore.getState();

            if (searchJobId === currentJobId && searchUiState === SEARCH_UI_STATE.QUERYING) {
                updateSearchUiState(SEARCH_UI_STATE.FAILED);
            }
        },
        parse: (data) => {
            // MongoDB ObjectIds are serialized as `{"$oid": "..."}` in the SSE stream.
            const doc = JSON.parse(data) as Omit<SearchResult, "_id"> &
                {_id: string | {$oid: string}};

            return {
                ...doc,
                _id: "object" === typeof doc._id ?
                    doc._id.$oid :
                    doc._id,
            };
        },
        rawDocs: true,
        sorted: true,
    });
};

export {useSearchResults};
