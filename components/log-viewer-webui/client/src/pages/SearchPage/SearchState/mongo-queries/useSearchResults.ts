import {useCursor} from "../../../../api/socket/useCursor";
import MongoCollectionSocket from "../../../../api/socket/MongoCollectionSocket";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../index";
import {
    SearchResult,
} from "../../SearchResults/SearchResultsTable/typings";
import { Nullable } from "../../../../typings/common";
import { SEARCH_UI_STATE } from "../typings";

/**
 * Custom hook to get results metadata for the current searchJobId.
 */
const useSearchResults = () => {
    const {searchJobId, searchUiState} = useSearchStore();
    console.log(`this is search jobID ${searchJobId}`);

    const searchResultsCursor = useCursor(
        () => {
            // If there is no active search job, there is no metadata to fetch. The cursor will
            // return null.
            if (searchJobId === SEARCH_STATE_DEFAULT.searchJobId ||
                searchUiState !== SEARCH_UI_STATE.DONE
            ) {
                return null;
            }
            const collection = new MongoCollectionSocket(searchJobId.toString());
            return collection.find({}, {});
        },
        [searchJobId, searchUiState]
    );

    return searchResultsCursor as Nullable<SearchResult[]>;
};

export { useSearchResults };

