import dayjs from "dayjs";
import {create} from "zustand";

import {
    DEFAULT_TIME_RANGE,
    TIME_RANGE_OPTION_DAYJS_MAP,
} from "../SearchControls/TimeRangeInput/utils";
import {SEARCH_UI_STATE} from "./typings";

import {SearchResultsMetadataDocument} from "@common/searchResultsMetadata.js";


/**
 * Default values of the search state.
 */
const SEARCH_STATE_DEFAULT = Object.freeze({
    queryString: "",
    searchJobId: null,
    aggregationJobId: null,
    searchUiState:  SEARCH_UI_STATE.DEFAULT,
    timeRange: TIME_RANGE_OPTION_DAYJS_MAP[DEFAULT_TIME_RANGE],
    searchResultsMetadata: null,
});

interface SearchState {
    queryString: string;
    timeRange: [dayjs.Dayjs, dayjs.Dayjs];
    searchJobId: string | null;
    aggregationJobId: string | null;
    searchUiState: SEARCH_UI_STATE;
    searchResultsMetadata: SearchResultsMetadataDocument | null;
    updateQueryString: (query: string) => void;
    updateTimeRange: (range: [dayjs.Dayjs, dayjs.Dayjs]) => void;
    updateSearchJobId: (id: string | null) => void;
    updateAggregationJobId: (id: string | null) => void;
    updateSearchUiState: (state: SEARCH_UI_STATE) => void;
    updateSearchResultsMetadata: (metadata: SearchResultsMetadataDocument) => void;
}

const useSearchStore = create<SearchState>((set) => ({
    queryString: SEARCH_STATE_DEFAULT.queryString,
    timeRange: SEARCH_STATE_DEFAULT.timeRange,
    searchJobId: SEARCH_STATE_DEFAULT.searchJobId,
    aggregationJobId: SEARCH_STATE_DEFAULT.aggregationJobId,
    searchUiState: SEARCH_STATE_DEFAULT.searchUiState,
    searchResultsMetadata: SEARCH_STATE_DEFAULT.searchResultsMetadata,
    updateQueryString: (query) => {
        set({queryString: query});
    },
    updateSearchUiState: (state) => {
        set({searchUiState: state});
    },
    updateTimeRange: (range) => {
        set({timeRange: range});
    },
    updateSearchJobId: (id) => {
        set({searchJobId: id});
    },
    updateAggregationJobId: (id) => {
        set({aggregationJobId: id});
    },
    updateSearchResultsMetadata: (metadata) => {
        set({searchResultsMetadata: metadata});
    },
}));


export {SEARCH_STATE_DEFAULT};
export default useSearchStore;
