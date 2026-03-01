import {Dayjs} from "dayjs";
import {create} from "zustand";

import {TimelineConfig} from "../../../components/ResultsTimeline/typings";
import {
    DEFAULT_TIME_RANGE,
    DEFAULT_TIME_RANGE_OPTION,
    TIME_RANGE_OPTION,
} from "../SearchControls/TimeRangeInput/utils";
import {computeTimelineConfig} from "../SearchResults/SearchResultsTimeline/utils";
import {SEARCH_UI_STATE} from "./typings";


/**
 * Default values of the search state.
 */
const SEARCH_STATE_DEFAULT = Object.freeze({
    aggregationJobId: null,
    numSearchResultsMetadata: 0,
    numSearchResultsTable: 0,
    numSearchResultsTimeline: 0,
    queriedDatasets: [],
    queryIsCaseSensitive: false,
    queryString: "",
    searchJobId: null,
    searchUiState: SEARCH_UI_STATE.DEFAULT,
    selectedDatasets: [],
    timeRange: DEFAULT_TIME_RANGE,
    timeRangeOption: DEFAULT_TIME_RANGE_OPTION,
    timelineConfig: computeTimelineConfig(DEFAULT_TIME_RANGE),
});

interface SearchState {
    /**
     * Unique ID from the database for the aggregation job.
     */
    aggregationJobId: string | null;

    /**
     * The number of search results from server metadata.
     */
    numSearchResultsMetadata: number;

    /**
     * The number of search table results.
     */
    numSearchResultsTable: number;

    /**
     * The number of timeline results.
     */
    numSearchResultsTimeline: number;

    /**
     * Datasets that were included in the most recently submitted query. Separate from
     * `selectedDatasets` so that post-submission UI changes don't affect in-flight query state.
     */
    queriedDatasets: string[];

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
     * Datasets currently selected in the UI dropdown.
     */
    selectedDatasets: string[];

    /**
     * Time range for search query.
     */
    timeRange: [Dayjs, Dayjs];

    /**
     * Time range preset.
     */
    timeRangeOption: TIME_RANGE_OPTION;

    /**
     * Time range and bucket duration for the timeline. The timeline config should
     * only be updated when queries are submitted and not when the range picker
     * selection is changed.
     */
    timelineConfig: TimelineConfig;

    updateAggregationJobId: (id: string | null) => void;
    updateNumSearchResultsMetadata: (num: number) => void;
    updateNumSearchResultsTable: (num: number) => void;
    updateNumSearchResultsTimeline: (num: number) => void;
    updateQueriedDatasets: (datasets: string[]) => void;
    updateQueryIsCaseSensitive: (newValue: boolean) => void;
    updateQueryString: (query: string) => void;
    updateSearchJobId: (id: string | null) => void;
    updateSearchUiState: (state: SEARCH_UI_STATE) => void;
    updateSelectedDatasets: (datasets: string[]) => void;
    updateTimeRange: (range: [Dayjs, Dayjs]) => void;
    updateTimeRangeOption: (option: TIME_RANGE_OPTION) => void;
    updateTimelineConfig: (config: TimelineConfig) => void;
}

const useSearchStore = create<SearchState>((set) => ({
    ...SEARCH_STATE_DEFAULT,
    updateAggregationJobId: (id) => {
        set({aggregationJobId: id});
    },
    updateNumSearchResultsMetadata: (num) => {
        set({numSearchResultsMetadata: num});
    },
    updateNumSearchResultsTable: (num) => {
        set({numSearchResultsTable: num});
    },
    updateNumSearchResultsTimeline: (num) => {
        set({numSearchResultsTimeline: num});
    },
    updateQueriedDatasets: (datasets) => {
        set({queriedDatasets: datasets});
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
    updateSelectedDatasets: (datasets) => {
        set({selectedDatasets: datasets});
    },
    updateTimeRange: (range) => {
        set({timeRange: range});
    },
    updateTimeRangeOption: (option: TIME_RANGE_OPTION) => {
        set({timeRangeOption: option});
    },
    updateTimelineConfig: (config) => {
        set({timelineConfig: config});
    },
}));


export {SEARCH_STATE_DEFAULT};
export default useSearchStore;
