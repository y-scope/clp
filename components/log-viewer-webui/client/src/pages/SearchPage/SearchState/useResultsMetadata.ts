import {SearchResultsMetadataDocument} from "@common/index.js";

import MongoSocketCollection from "../../../api/socket/MongoSocketCollection";
import {useCursor} from "../../../api/socket/useCursor";
import {settings} from "../../../settings";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "./index";


/**
 * Custom hook to get result metadata for the current searchJobId.
 *
 * @return
 */
const useResultsMetadata = () => {
    const {searchJobId} = useSearchStore();

    const resultsMetadataCursor = useCursor<SearchResultsMetadataDocument>(
        () => {
            // If there is no active search job, there is no metadata to fetch.
            if (searchJobId === SEARCH_STATE_DEFAULT.searchJobId
            ) {
                return null;
            }

            const collection = new MongoSocketCollection(
                settings.MongoDbSearchResultsMetadataCollectionName
            );

            console.log(
                `Subscribing to updates for results metadata for search job ID: ${searchJobId}`
            );

            return collection.find({_id: searchJobId.toString()}, {limit: 1});
        },
        [searchJobId]
    );

    // If there is no metadata, return null.
    if (null === resultsMetadataCursor ||
        (Array.isArray(resultsMetadataCursor) && 0 === resultsMetadataCursor.length)
    ) {
        return null;
    }

    const [resultsMetadata] = resultsMetadataCursor;

    if ("undefined" === typeof resultsMetadata) {
        return null;
    }


    return resultsMetadata;
};


export {useResultsMetadata};
