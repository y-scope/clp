import {Nullable} from "@webui/common/utility-types";
import dayjs, {Dayjs} from "dayjs";
import {create} from "zustand";

import {TimelineConfig} from "../../../components/ResultsTimeline/typings";
import {
    DEFAULT_TIME_RANGE,
    TIME_RANGE_OPTION,
    TIME_RANGE_SINCE_UNIX_EPOCH,
} from "../SearchControls/TimeRangeInput/utils";
import {computeTimelineConfig} from "../SearchResults/SearchResultsTimeline/utils";
import {SEARCH_UI_STATE} from "./typings";


/**
 * Default values of the search state.
 */
const SEARCH_STATE_DEFAULT = Object.freeze({
    aggregationJobId: null,
    cachedDataset: null,
    numSearchResultsMetadata: 0,
    numSearchResultsTable: 0,
    numSearchResultsTimeline: 0,
    queryIsCaseSensitive: false,
    queryString: "",
    searchJobId: null,
    searchUiState: SEARCH_UI_STATE.DEFAULT,
    selectDataset: null,
    timeRange: TIME_RANGE_SINCE_UNIX_EPOCH,
    timeRangeOption: DEFAULT_TIME_RANGE,
    timelineConfig: computeTimelineConfig(TIME_RANGE_SINCE_UNIX_EPOCH),
});

interface SearchState {
    /**
     * Unique ID from the database for the aggregation job.
     */
    aggregationJobId: string | null;

    /**
     * Clp-s dataset filter submitted as part of query. There is a separate state for the submitted
     * dataset so modifications to the selector do not change dataset used in extract stream job for
     * log viewer links.
     */
    cachedDataset: Nullable<string>;

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
     * Clp-s dataset filter shown in UI selector.
     */
    selectDataset: string | null;

    /**
     * Time range for search query.
     */
    timeRange: [dayjs.Dayjs, dayjs.Dayjs];

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
    updateCachedDataset: (dataset: string) => void;
    updateNumSearchResultsMetadata: (num: number) => void;
    updateNumSearchResultsTable: (num: number) => void;
    updateNumSearchResultsTimeline: (num: number) => void;
    updateQueryIsCaseSensitive: (newValue: boolean) => void;
    updateQueryString: (query: string) => void;
    updateSearchJobId: (id: string | null) => void;
    updateSearchUiState: (state: SEARCH_UI_STATE) => void;
    updateSelectDataset: (dataset: string | null) => void;
    updateTimeRange: (range: [dayjs.Dayjs, dayjs.Dayjs]) => void;
    updateTimeRangeOption: (option: TIME_RANGE_OPTION) => void;
    updateTimelineConfig: (config: TimelineConfig) => void;
}

const useSearchStore = create<SearchState>((set) => ({
    ...SEARCH_STATE_DEFAULT,
    updateAggregationJobId: (id) => {
        set({aggregationJobId: id});
    },
    updateCachedDataset: (dataset) => {
        set({cachedDataset: dataset});
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
    updateSelectDataset: (dataset) => {
        set({selectDataset: dataset});
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
