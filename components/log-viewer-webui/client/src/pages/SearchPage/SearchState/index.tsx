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
    aggregationJobId: null,
    queryIsCaseSensitive: false,
    queryString: "",
    searchJobId: null,
    searchResultsMetadata: null,
    searchUiState: SEARCH_UI_STATE.DEFAULT,
    timeRange: TIME_RANGE_OPTION_DAYJS_MAP[DEFAULT_TIME_RANGE],
    timeRangeOption: DEFAULT_TIME_RANGE,
});

interface SearchState {
    /**
     * Unique ID from the database for the aggregation job.
     */
    aggregationJobId: string | null;

    /**
     * Whether the query is case sensitive.
     */
    queryIsCaseSensitive: boolean;

    /**
     * The search query string.
     */
    queryString: string;

    /**
     * Unique ID from the database for the search job.
     */
    searchJobId: string | null;

    /**
     * UI state of search page.
     */
    searchUiState: SEARCH_UI_STATE;

    /**
     * Time range for search query.
     */
    timeRange: [dayjs.Dayjs, dayjs.Dayjs];

    /**
     * Time range preset.
     */
    timeRangeOption: TIME_RANGE_OPTION;

    updateAggregationJobId: (id: string | null) => void;
    updateQueryIsCaseSensitive: (newValue: boolean) => void;
    updateQueryString: (query: string) => void;
    updateSearchJobId: (id: string | null) => void;
    updateSearchUiState: (state: SEARCH_UI_STATE) => void;
    updateTimeRange: (range: [dayjs.Dayjs, dayjs.Dayjs]) => void;
    updateTimeRangeOption: (option: TIME_RANGE_OPTION) => void;
}

const useSearchStore = create<SearchState>((set) => ({
    ...SEARCH_STATE_DEFAULT,
    updateAggregationJobId: (id) => {
        set({aggregationJobId: id});
    },
    updateQueryIsCaseSensitive: (newValue: boolean) => {
        set({queryIsCaseSensitive: newValue});
    },
    updateQueryString: (query) => {
        set({queryString: query});
    },
    updateSearchJobId: (id) => {
        set({searchJobId: id});
    },
    updateSearchUiState: (state) => {
        set({searchUiState: state});
    },
    updateTimeRange: (range) => {
        set({timeRange: range});
    },
    updateTimeRangeOption: (option: TIME_RANGE_OPTION) => {
        set({timeRangeOption: option});
    },
}));


export {SEARCH_STATE_DEFAULT};
export default useSearchStore;
