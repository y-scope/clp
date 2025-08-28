import type {PrestoSearchResult} from "@webui/common";

import MongoSocketCollection from "../../../../../../api/socket/MongoSocketCollection";
import {useCursor} from "../../../../../../api/socket/useCursor";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../../../SearchState/index";
import {SEARCH_MAX_NUM_RESULTS} from "../../typings";


/**
 * Custom hook to get Presto search results for the current searchJobId.
 *
 * @return
 */
const usePrestoSearchResults = () => {
    const searchJobId = useSearchStore((state) => state.searchJobId);

    const searchResultsCursor = useCursor<PrestoSearchResult>(
        () => {
            // If there is no active search job, there are no results to fetch. The cursor will
            // return null.
            if (searchJobId === SEARCH_STATE_DEFAULT.searchJobId) {
                return null;
            }

            console.log(
                `Subscribing to updates to Presto search results with job ID: ${searchJobId}`
            );

            // Retrieve 1k most recent results.
            const options = {
                sort: [
                    [
                        "_id",
                        "desc",
                    ],
                ],
                limit: SEARCH_MAX_NUM_RESULTS,
            };

            const collection = new MongoSocketCollection(searchJobId);
            return collection.find({}, options);
        },
        [searchJobId]
    );

    return searchResultsCursor;
};

export {usePrestoSearchResults};
