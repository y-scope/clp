import dayjs from "dayjs";
import {create} from "zustand";

import {
    DEFAULT_TIME_RANGE,
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
} from "../SearchControls/TimeRangeInput/utils";

import {SEARCH_UI_STATE} from "./typings";

/**
 * Default values of the search state.
 */
const SEARCH_STATE_DEFAULT = Object.freeze({
    queryString: "",
    timeRange: TIME_RANGE_OPTION_DAYJS_MAP[DEFAULT_TIME_RANGE],
    timeRangeOption: DEFAULT_TIME_RANGE,
    searchJobId: null,
    aggregationJobId: null,
    searchUiState:  SEARCH_UI_STATE.DEFAULT,
    searchResultsMetadata: null,
});

interface SearchState {
    /**
     * The search query string.
     */
    queryString: string;
    /**
     * Time range for search query.
     */
    timeRange: [dayjs.Dayjs, dayjs.Dayjs];
    /**
     * Time range preset.
     */
    timeRangeOption: TIME_RANGE_OPTION;
    /**
     * Unique ID from the database for the search job.
     */
    searchJobId: string | null;
    /**
     * Unique ID from the database for the aggregation job.
     */
    aggregationJobId: string | null;
    /**
     * UI state of search page.
     */
    searchUiState: SEARCH_UI_STATE;
    updateQueryString: (query: string) => void;
    updateTimeRange: (range: [dayjs.Dayjs, dayjs.Dayjs]) => void;
    updateTimeRangeOption: (option: TIME_RANGE_OPTION) => void;
    updateSearchJobId: (id: string | null) => void;
    updateAggregationJobId: (id: string | null) => void;
    updateSearchUiState: (state: SEARCH_UI_STATE) => void;
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
}));


export {SEARCH_STATE_DEFAULT};
export default useSearchStore;
