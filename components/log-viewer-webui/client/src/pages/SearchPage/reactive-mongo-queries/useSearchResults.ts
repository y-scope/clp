import MongoCollectionSocket from "../../../api/socket/MongoCollectionSocket";
import {useCursor} from "../../../api/socket/useCursor";
import {SearchResult} from "../SearchResults/SearchResultsTable/typings";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../SearchState/index";


/**
 * Custom hook to get search results for the current searchJobId.
 *
 * @return
 */
const useSearchResults = () => {
    const {searchJobId} = useSearchStore();

    const searchResultsCursor = useCursor<SearchResult>(
        () => {
            // If there is no active search job, there is no metadata to fetch. The cursor will
            // return null.
            if (searchJobId === SEARCH_STATE_DEFAULT.searchJobId) {
                return null;
            }

            const collection = new MongoCollectionSocket(searchJobId.toString());
            return collection.find({}, {});
        },
        [searchJobId]
    );

    return searchResultsCursor;
};

export {useSearchResults};
