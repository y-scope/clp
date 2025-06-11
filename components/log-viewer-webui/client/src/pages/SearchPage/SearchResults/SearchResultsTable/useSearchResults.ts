import MongoCollectionSocket from "../../../../api/socket/MongoCollectionSocket";
import {useCursor} from "../../../../api/socket/useCursor";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../SearchState/index";
import {SearchResult, SEARCH_MAX_NUM_RESULTS} from "./typings";


/**
 * Custom hook to get search results for the current searchJobId.
 *
 * @return
 */
const useSearchResults = () => {
    const {searchJobId} = useSearchStore();

    const searchResultsCursor = useCursor<SearchResult>(
        () => {
            // If there is no active search job, there are no results to fetch. The cursor will
            // return null.
            if (searchJobId === SEARCH_STATE_DEFAULT.searchJobId) {
                return null;
            }

            //Retrieve 1k most recent results.
            const findOptions = {
                sort:[["timestamp", "desc"], ["id", "desc"]],
                limit: SEARCH_MAX_NUM_RESULTS,
            };

            const collection = new MongoCollectionSocket(searchJobId.toString());
            return collection.find({}, findOptions);
        },
        [searchJobId]
    );

    return searchResultsCursor;
};

export {useSearchResults};
