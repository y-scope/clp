import {useCursor} from "../../../../api/socket/useCursor";
import MongoCollectionSocket from "../../../../api/socket/MongoCollectionSocket";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../index";
import { SearchResultsMetadataDocument } from "@common/index.js";
import { Nullable } from "../../../../typings/common";

import settings from "../../../../../settings.json" with { type: "json" };

/**
 * Custom hook to get results metadata for the current searchJobId.
 */
const useResultsMetadata = () => {
    const {searchJobId} = useSearchStore();

    console.log(`Requesting metadata for searchJobID:${searchJobId}`);
    const resultsMetadataCursor = useCursor(
        () => {
            // If there is no active search job, there is no metadata to fetch. The cursor will
            // return null.
            if (searchJobId === SEARCH_STATE_DEFAULT.searchJobId
            ) {
                return null;
            }

            const collection = new MongoCollectionSocket(
                settings.MongoDbSearchResultsMetadataCollectionName
            );
            return collection.find({ _id: searchJobId.toString() }, { limit: 1 });
        },
        [searchJobId]
    );

    return resultsMetadataCursor as Nullable<SearchResultsMetadataDocument[]>;
};

export { useResultsMetadata };

// Before we send delete request. We somehow