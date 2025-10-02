import {type PrestoQueryJob} from "@webui/common/schemas/presto-search";

import {
    cancelQuery,
    clearQueryResults,
    submitQuery,
} from "../../../../api/presto-search";
import {
    buildSearchQuery,
    buildTimelineQuery,
} from "../../../../sql-parser";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../SearchState";
import usePrestoSearchState from "../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../SearchState/typings";


const PRESTO_TIMELINE_BUCKET_COUNT = 40;

/**
 * Clears current presto query results on server.
 */
const handlePrestoClearResults = () => {
    const {searchUiState, searchJobId} = useSearchStore.getState();

    // In the starting state, there are no results to clear.
    if (searchUiState === SEARCH_UI_STATE.DEFAULT) {
        return;
    }

    if (null === searchJobId) {
        console.error("Cannot clear results: searchJobId is not set.");

        return;
    }

    clearQueryResults(
        {searchJobId}
    ).catch((err: unknown) => {
        console.error("Failed to clear query results:", err);
    });
};

/**
 * Build the search query and timeline query for guided Presto search.
 *
 * @return
 * @throws {Error} if any component is missing
 */
const buildPrestoQueries = () => {
    const {timeRange} = useSearchStore.getState();
    const [startTimestamp, endTimestamp] = timeRange;
    const {select, from, where, orderBy, limit, timestampKey} = usePrestoSearchState.getState();

    if (null === from) {
        throw new Error("Cannot build guided query: from input is missing");
    }

    if (null === timestampKey) {
        throw new Error("Cannot build guided query: timestampKey input is missing");
    }

    const trimmedWhere = where.trim();
    const trimmedOrderBy = orderBy.trim();
    const limitString = String(limit);

    const searchQueryString = buildSearchQuery({
        ...(trimmedWhere && {booleanExpression: trimmedWhere}),
        databaseName: from,
        endTimestamp: endTimestamp.unix(),
        ...(limitString && {limitValue: limitString}),
        selectItemList: select.trim(),
        ...(trimmedOrderBy && {sortItemList: trimmedOrderBy}),
        startTimestamp: startTimestamp.unix(),
        timestampKey: timestampKey,
    });

    const timelineQueryString = buildTimelineQuery({
        bucketCount: PRESTO_TIMELINE_BUCKET_COUNT,
        databaseName: from,
        endTimestamp: endTimestamp.unix(),
        startTimestamp: startTimestamp.unix(),
        timestampKey: timestampKey,
        ...(trimmedWhere && {booleanExpression: trimmedWhere}),
    });

    return {searchQueryString, timelineQueryString};
};

/**
 * Submits a new Presto query to server.
 */
const handlePrestoQuerySubmit = () => {
    const {searchQueryString, timelineQueryString} = buildPrestoQueries();

    const {
        updateAggregationJobId,
        updateNumSearchResultsTable,
        updateNumSearchResultsMetadata,
        updateSearchJobId,
        updateSearchUiState,
        searchUiState,
    } = useSearchStore.getState();

    // User should NOT be able to submit a new query while an existing query is in progress.
    if (
        searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        searchUiState !== SEARCH_UI_STATE.DONE &&
        searchUiState !== SEARCH_UI_STATE.FAILED
    ) {
        console.error("Cannot submit query while existing query is in progress.");

        return;
    }

    handlePrestoClearResults();

    updateNumSearchResultsTable(SEARCH_STATE_DEFAULT.numSearchResultsTable);
    updateNumSearchResultsMetadata(SEARCH_STATE_DEFAULT.numSearchResultsMetadata);
    updateSearchUiState(SEARCH_UI_STATE.QUERY_ID_PENDING);

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
 * Cancels an ongoing Presto search query on server.
 *
 * @param payload
 */
const handlePrestoQueryCancel = (payload: PrestoQueryJob) => {
    const {searchUiState, updateSearchUiState} = useSearchStore.getState();
    if (searchUiState !== SEARCH_UI_STATE.QUERYING) {
        console.error("Cannot cancel query if there is no ongoing query.");

        return;
    }

    updateSearchUiState(SEARCH_UI_STATE.DONE);
    cancelQuery(payload)
        .then(() => {
            console.debug("Query cancelled successfully");
        })
        .catch((err: unknown) => {
            console.error("Failed to cancel query:", err);
        });
};

export {
    handlePrestoQueryCancel,
    handlePrestoQuerySubmit,
};
