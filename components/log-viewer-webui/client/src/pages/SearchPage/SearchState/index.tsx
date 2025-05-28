import dayjs from "dayjs";
import {create} from "zustand";

import {
    DEFAULT_TIME_RANGE,
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
} from "../SearchControls/TimeRangeInput/utils";
import {SEARCH_UI_STATE} from "./typings";

import { TimelineConfig } from "../../../components/ResultsTimeline/typings";
import { computeTimelineConfig } from "../SearchResults/SearchResultsTimeline/utils";


/**
 * Default values of the search state.
 */
const SEARCH_STATE_DEFAULT = Object.freeze({
    aggregationJobId: null,
    queryString: "",
    searchJobId: null,
    searchResultsMetadata: null,
    searchUiState: SEARCH_UI_STATE.DEFAULT,
    timelineConfig: computeTimelineConfig(TIME_RANGE_OPTION_DAYJS_MAP[DEFAULT_TIME_RANGE]),
    timeRange: TIME_RANGE_OPTION_DAYJS_MAP[DEFAULT_TIME_RANGE],
    timeRangeOption: DEFAULT_TIME_RANGE,
});

interface SearchState {
    /**
     * Unique ID from the database for the aggregation job.
     */
    aggregationJobId: string | null;

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
     * Time range and bucket duration for the timeline. The timeline config should
     * only be updated when queries are submitted and not when the range picker
     * selection is changed.
     */
    timelineConfig: TimelineConfig;

    /**
     * Time range for search query.
     */
    timeRange: [dayjs.Dayjs, dayjs.Dayjs];

    /**
     * Time range preset.
     */
    timeRangeOption: TIME_RANGE_OPTION;

    updateAggregationJobId: (id: string | null) => void;
    updateQueryString: (query: string) => void;
    updateSearchJobId: (id: string | null) => void;
    updateSearchUiState: (state: SEARCH_UI_STATE) => void;
    updateTimelineConfig: (config: TimelineConfig) => void;
    updateTimeRange: (range: [dayjs.Dayjs, dayjs.Dayjs]) => void;
    updateTimeRangeOption: (option: TIME_RANGE_OPTION) => void;
}

const useSearchStore = create<SearchState>((set) => ({
    ...SEARCH_STATE_DEFAULT,
    updateAggregationJobId: (id) => {
        set({aggregationJobId: id});
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
    updateTimelineConfig: (config) => {
        set({timelineConfig: config});
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
