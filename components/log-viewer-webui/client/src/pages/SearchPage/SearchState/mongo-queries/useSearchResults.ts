import {useCursor} from "../../../../api/socket/useCursor";
import MongoCollectionSocket from "../../../../api/socket/MongoCollectionSocket";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../index";
import {
    SearchResult,
} from "../../SearchResults/SearchResultsTable/typings";
import { Nullable } from "../../../../typings/common";

/**
 * Custom hook to get results metadata for the current searchJobId.
 */
const useSearchResults = () => {
    const {searchJobId} = useSearchStore();
    console.log(`this is search jobID ${searchJobId}`);

    const searchResultsCursor = useCursor(
        () => {
            // If there is no active search job, there is no metadata to fetch. The cursor will
            // return null.
            if (searchJobId === SEARCH_STATE_DEFAULT.searchJobId ) {
                return null;
            }

            const collection = new MongoCollectionSocket(searchJobId.toString());
            return collection.find({}, {});
        },
        [searchJobId]
    );

    return searchResultsCursor as Nullable<SearchResult[]>;
};

export { useSearchResults };


// Call Clear, which will delete the mongo collection.
// But the searchResults are still monitoring it.
// We need to first unsubsribe.
// When the query Finishes, we should drop the cursor?

// when the query is done. we stop getting updates.
// If we cancel