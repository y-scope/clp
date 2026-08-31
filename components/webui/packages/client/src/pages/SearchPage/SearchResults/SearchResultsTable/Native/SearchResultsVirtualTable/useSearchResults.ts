import {useQueryResults} from "@webui/api-client/useQueryResults";

import {apiClient} from "../../../../../../api/search";
import useSearchStore from "../../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {SearchResult} from "./typings";


interface SearchResultId {
    archive_id: string;
    log_event_idx: number;
}

interface LegacyObjectId {
    $oid: string;
}

type RawSearchResult =
    (Omit<SearchResult, "_id" | "log_event_idx"> & {
        _id: LegacyObjectId | string;
        log_event_ix: number;
    }) |
    (Omit<SearchResult, "_id" | "archive_id" | "log_event_idx"> & {
        _id: SearchResultId;
    });


/**
 * Custom hook to stream search results for the current searchJobId from the API server's SSE
 * endpoint. When the stream ends, the search UI state is updated to `DONE` (or `FAILED` if the
 * stream errors out), unless the query has already been cancelled.
 *
 * @return
 */
const useSearchResults = () => {
    const searchJobId = useSearchStore((state) => state.searchJobId);

    return useQueryResults<SearchResult>(apiClient, searchJobId, {
        // Sort by timestamp (desc) to match the previous MongoDB cursor ordering.
        compareFn: (a, b) => b.timestamp - a.timestamp,
        onDone: () => {
            const {searchUiState, updateSearchUiState} = useSearchStore.getState();
            if (searchUiState === SEARCH_UI_STATE.QUERYING) {
                updateSearchUiState(SEARCH_UI_STATE.DONE);
            }
        },
        onError: (err) => {
            console.error("Failed to stream search results:", err);
            const {searchUiState, updateSearchUiState} = useSearchStore.getState();
            if (searchUiState === SEARCH_UI_STATE.QUERYING) {
                updateSearchUiState(SEARCH_UI_STATE.FAILED);
            }
        },
        parse: (data) => {
            const doc = JSON.parse(data) as RawSearchResult;

            if ("log_event_ix" in doc) {
                return {
                    ...doc,
                    _id: "object" === typeof doc._id ?
                        doc._id.$oid :
                        doc._id,
                    log_event_idx: doc.log_event_ix,
                };
            }

            return {
                ...doc,
                archive_id: doc._id.archive_id,
                _id: JSON.stringify(doc._id),
                log_event_idx: doc._id.log_event_idx,
            };
        },
        rawDocs: true,
        sorted: true,
    });
};

export {useSearchResults};
