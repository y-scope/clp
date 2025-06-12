import MongoSocketCollection from "../../../../api/socket/MongoSocketCollection";
import {useCursor} from "../../../../api/socket/useCursor";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../SearchState/index";
import {
    SEARCH_MAX_NUM_RESULTS,
    SearchResult,
} from "./typings";


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

            console.log(
                `Subscribing to updates to search results with job ID: ${searchJobId}`
            );

            // Retrieve 1k most recent results.
            const options = {
                sort: [
                    [
                        "timestamp",
                        "desc",
                    ],
                    [
                        "_id",
                        "desc",
                    ],
                ],
                limit: SEARCH_MAX_NUM_RESULTS,
            };

            const collection = new MongoSocketCollection(searchJobId.toString());
            return collection.find({}, options);
        },
        [searchJobId]
    );

    return searchResultsCursor;
};

export {useSearchResults};
