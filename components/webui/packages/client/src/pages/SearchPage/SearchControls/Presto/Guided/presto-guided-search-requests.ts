import {Dayjs} from "dayjs";

import {
    cancelQuery,
    clearQueryResults,
    submitQuery,
} from "../../../../../api/presto-search";
import {MAX_DATA_POINTS_PER_TIMELINE} from "../../../../../components/ResultsTimeline/typings";
import {
    buildSearchQuery,
    buildTimelineQuery,
} from "../../../../../sql-parser";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../../SearchState";
import usePrestoSearchState, {PRESTO_SEARCH_STATE_DEFAULT} from "../../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../../SearchState/Presto/typings";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";


/**
 * Default limit for presto search query
 */
const DEFAULT_SEARCH_LIMIT = 1000;


/**
 * Clears current presto guided query results on server.
 */
const handlePrestoGuidedClearResults = () => {
    const {searchUiState, searchJobId, aggregationJobId} = useSearchStore.getState();

    // In the starting state, there are no results to clear.
    if (searchUiState === SEARCH_UI_STATE.DEFAULT) {
        return;
    }

    if (null === searchJobId) {
        console.error("Cannot clear results: searchJobId is not set.");

        return;
    }

    if (null === aggregationJobId) {
        console.error("Cannot clear results: aggregationJobId is not set.");

        return;
    }

    clearQueryResults(
        {searchJobId: searchJobId}
    ).catch((err: unknown) => {
        console.error("Failed to clear query results:", err);
    });

    clearQueryResults(
        {searchJobId: aggregationJobId}
    ).catch((err: unknown) => {
        console.error("Failed to clear aggregation results:", err);
    });
};

/**
 * Build the search query and timeline query for guided Presto search.
 *
 * @param timeRange
 * @return
 * @throws {Error} if any component is missing
 */
const buildPrestoGuidedQueries = (timeRange: [Dayjs, Dayjs]) => {
    const [startTimestamp, endTimestamp] = timeRange;
    const {select, where, orderBy, timestampKey} = usePrestoSearchState.getState();
    const {selectedDatasets} = useSearchStore.getState();
    const [from] = selectedDatasets;

    if ("undefined" === typeof from) {
        throw new Error("Cannot build guided query: from input is missing");
    }

    if (null === timestampKey) {
        throw new Error("Cannot build guided query: timestampKey input is missing");
    }

    const trimmedWhere = where.trim();
    const trimmedOrderBy = orderBy.trim();

    const searchQueryString = buildSearchQuery({
        ...(trimmedWhere && {booleanExpression: trimmedWhere}),
        databaseName: from,
        endTimestamp: endTimestamp,
        selectItemList: select.trim(),
        ...(trimmedOrderBy && {sortItemList: trimmedOrderBy}),
        limitValue: String(DEFAULT_SEARCH_LIMIT),
        startTimestamp: startTimestamp,
        timestampKey: timestampKey,
    });

    const timelineQueryString = buildTimelineQuery({
        bucketCount: MAX_DATA_POINTS_PER_TIMELINE,
        databaseName: from,
        endTimestamp: endTimestamp,
        startTimestamp: startTimestamp,
        timestampKey: timestampKey,
        ...(trimmedWhere && {booleanExpression: trimmedWhere}),
    });

    return {searchQueryString, timelineQueryString};
};


/**
 * Submits guided Presto queries to server.
 *
 * @param searchQueryString
 * @param timelineQueryString
 */
const handlePrestoGuidedQuerySubmit = (searchQueryString: string, timelineQueryString: string) => {
    const {
        updateNumSearchResultsTable,
        updateNumSearchResultsTimeline,
        updateNumSearchResultsMetadata,
        updateSearchJobId,
        updateSearchUiState,
        searchUiState,
    } = useSearchStore.getState();

    const {
        updateErrorMsg, updateErrorName,
    } = usePrestoSearchState.getState();

    // User should NOT be able to submit a new query while an existing query is in progress.
    if (
        searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        searchUiState !== SEARCH_UI_STATE.DONE &&
        searchUiState !== SEARCH_UI_STATE.FAILED
    ) {
        console.error("Cannot submit query while existing query is in progress.");

        return;
    }

    handlePrestoGuidedClearResults();

    updateNumSearchResultsTable(SEARCH_STATE_DEFAULT.numSearchResultsTable);
    updateNumSearchResultsTimeline(SEARCH_STATE_DEFAULT.numSearchResultsTimeline);
    updateNumSearchResultsMetadata(SEARCH_STATE_DEFAULT.numSearchResultsMetadata);
    updateSearchUiState(SEARCH_UI_STATE.QUERY_ID_PENDING);

    updateErrorMsg(null);
    updateErrorName(null);

    submitQuery({queryString: searchQueryString})
        .then((result) => {
            const {searchJobId} = result.data;
            updateSearchJobId(searchJobId);
            updateSearchUiState(SEARCH_UI_STATE.QUERYING);
            console.debug(
                "Presto search job created - ",
                "Search job ID:",
                searchJobId
            );
        })
        .catch((err: unknown) => {
            console.error("Failed to submit query:", err);
        });

    submitQuery({queryString: timelineQueryString})
        .then((result) => {
            const {updateAggregationJobId} = useSearchStore.getState();

            const {searchJobId: aggregationJobId} = result.data;
            updateAggregationJobId(aggregationJobId);
            console.debug(
                "Presto aggregation job created - ",
                "Aggregation job ID:",
                aggregationJobId
            );
        })
        .catch((err: unknown) => {
            console.error("Failed to submit aggregation query:", err);
        });
};

/**
 * Cancels ongoing Presto guided queries on server.
 *
 * @param searchJobId
 * @param aggregationJobId
 */
const handlePrestoGuidedQueryCancel = (searchJobId: string, aggregationJobId: string) => {
    const {searchUiState, updateSearchUiState} = useSearchStore.getState();
    if (searchUiState !== SEARCH_UI_STATE.QUERYING) {
        console.error("Cannot cancel query if there is no ongoing query.");

        return;
    }

    updateSearchUiState(SEARCH_UI_STATE.DONE);
    cancelQuery({searchJobId: searchJobId})
        .then(() => {
            console.debug("Query cancelled successfully");
        })
        .catch((err: unknown) => {
            console.error("Failed to cancel query:", err);
        });

    cancelQuery({searchJobId: aggregationJobId})
        .then(() => {
            console.debug("Query cancelled successfully");
        })
        .catch((err: unknown) => {
            console.error("Failed to cancel query:", err);
        });
};

/**
 * Handles switching to freeform SQL interface by clearing results and resetting states.
 */
const handleSwitchToFreeform = () => {
    const {
        searchUiState,
        updateSearchUiState,
        updateSearchJobId,
        updateAggregationJobId,
        updateNumSearchResultsTable,
        updateNumSearchResultsTimeline,
        updateNumSearchResultsMetadata,
    } = useSearchStore.getState();
    const {
        setSqlInterface,
        updateErrorMsg,
        updateErrorName,
        updateCachedGuidedSearchQueryString,
        updateQueryDrawerOpen,
    } = usePrestoSearchState.getState();

    setSqlInterface(PRESTO_SQL_INTERFACE.FREEFORM);

    // If already in default state, nothing to clear
    if (searchUiState === SEARCH_UI_STATE.DEFAULT) {
        return;
    }

    handlePrestoGuidedClearResults();

    updateSearchJobId(SEARCH_STATE_DEFAULT.searchJobId);
    updateAggregationJobId(SEARCH_STATE_DEFAULT.aggregationJobId);
    updateNumSearchResultsTable(SEARCH_STATE_DEFAULT.numSearchResultsTable);
    updateNumSearchResultsTimeline(SEARCH_STATE_DEFAULT.numSearchResultsTimeline);
    updateNumSearchResultsMetadata(SEARCH_STATE_DEFAULT.numSearchResultsMetadata);

    updateSearchUiState(SEARCH_UI_STATE.DEFAULT);

    updateErrorMsg(PRESTO_SEARCH_STATE_DEFAULT.errorMsg);
    updateErrorName(PRESTO_SEARCH_STATE_DEFAULT.errorName);
    updateCachedGuidedSearchQueryString(PRESTO_SEARCH_STATE_DEFAULT.cachedGuidedSearchQueryString);
    updateQueryDrawerOpen(PRESTO_SEARCH_STATE_DEFAULT.queryDrawerOpen);
};

export {
    buildPrestoGuidedQueries,
    handlePrestoGuidedQueryCancel,
    handlePrestoGuidedQuerySubmit,
    handleSwitchToFreeform,
};
