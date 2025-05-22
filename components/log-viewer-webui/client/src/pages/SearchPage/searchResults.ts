import {useCursor} from "../../api/socket/useCursor";
import MongoCollectionSocket from "../../api/socket/MongoCollectionSocket";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "./SearchState";

/**
 * Custom hook to get results metadata for the current searchJobId.
 */
const useSearchResults = () => {
    const searchJobId = useSearchStore((state) => state.searchJobId);
    console.log(`this is search jobID ${searchJobId}`);

    const resultsMetadataCursor = useCursor(
        () => {
            if (!searchJobId || searchJobId === SEARCH_STATE_DEFAULT.searchJobId) {
                return null;
            }
            const collection = new MongoCollectionSocket(searchJobId.toString());
            return collection.find({}, {});
        },
        [searchJobId]
    );

    return resultsMetadataCursor;
};

export { useSearchResults };

