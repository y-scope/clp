import { Nullable } from "../../../../../typings/common.js";
import { SEARCH_SIGNAL } from "../constants.js";

interface SearchResultsMetadataDocument {
    _id: string;
    errorMsg: Nullable<string>;
    lastSignal: SEARCH_SIGNAL;
    numTotalResults?: number;
}

export { SearchResultsMetadataDocument };
