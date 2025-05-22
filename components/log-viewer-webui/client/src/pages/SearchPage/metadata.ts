import {useCursor} from "../../api/socket/useCursor";
import MongoCollectionSocket from "../../api/socket/MongoCollectionSocket";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "./SearchState/index";
import { SearchResultsMetadataDocument } from "@common/searchResultsMetadata";


/**
 * Custom hook to get results metadata for the current searchJobId.
 */
const useResultsMetadata = () => {
    const searchJobId = useSearchStore((state) => state.searchJobId);
    console.log(`this is search jobID ${searchJobId}`);

    const resultsMetadataCursor = useCursor(
        () => {
            if (!searchJobId || searchJobId === SEARCH_STATE_DEFAULT.searchJobId) {
                return null;
            }
            const collection = new MongoCollectionSocket("results-metadata");
            return collection.find({ _id: searchJobId?.toString() }, { limit: 1 });
        },
        [searchJobId]
    );

    return resultsMetadataCursor as SearchResultsMetadataDocument[];
};

export { useResultsMetadata };


