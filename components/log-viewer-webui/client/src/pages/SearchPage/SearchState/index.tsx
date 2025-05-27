import dayjs from "dayjs";
import {create} from "zustand";

import {
    DEFAULT_TIME_RANGE,
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
} from "../SearchControls/TimeRangeInput/utils";

import {SEARCH_UI_STATE} from "./typings";

import {SearchResultsMetadataDocument} from "@common/index.js";


/**
 * Default values of the search state.
 */
const SEARCH_STATE_DEFAULT = Object.freeze({
    queryString: "",
    searchJobId: null,
    aggregationJobId: null,
    searchUiState:  SEARCH_UI_STATE.DEFAULT,
    timeRange: TIME_RANGE_OPTION_DAYJS_MAP[DEFAULT_TIME_RANGE],
    timeRangeOption: DEFAULT_TIME_RANGE,
    searchResultsMetadata: null,
});

interface SearchState {
    queryString: string;
    timeRange: [dayjs.Dayjs, dayjs.Dayjs];
    timeRangeOption: TIME_RANGE_OPTION;
    searchJobId: string | null;
    aggregationJobId: string | null;
    searchUiState: SEARCH_UI_STATE;
    searchResultsMetadata: SearchResultsMetadataDocument | null;
    updateQueryString: (query: string) => void;
    updateTimeRange: (range: [dayjs.Dayjs, dayjs.Dayjs]) => void;
    updateTimeRangeOption: (option: TIME_RANGE_OPTION) => void;
    updateSearchJobId: (id: string | null) => void;
    updateAggregationJobId: (id: string | null) => void;
    updateSearchUiState: (state: SEARCH_UI_STATE) => void;
    updateSearchResultsMetadata: (metadata: SearchResultsMetadataDocument) => void;
}

const useSearchStore = create<SearchState>((set) => ({
    ...SEARCH_STATE_DEFAULT,
    updateQueryString: (query) => {
        set({queryString: query});
    },
    updateTimeRange: (range) => {
        set({timeRange: range});
    },
    updateTimeRangeOption: (option: TIME_RANGE_OPTION) => {
        set({timeRangeOption: option});
    },
    updateSearchJobId: (id) => {
        set({searchJobId: id});
    },
    updateAggregationJobId: (id) => {
        set({aggregationJobId: id});
    },
    updateSearchUiState: (state) => {
        set({searchUiState: state});
    },
    updateSearchResultsMetadata: (metadata) => {
        set({searchResultsMetadata: metadata});
    },
}));


export {SEARCH_STATE_DEFAULT};
export default useSearchStore;
