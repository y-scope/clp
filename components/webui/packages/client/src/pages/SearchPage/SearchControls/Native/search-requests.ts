import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {
    type QueryJob,
    type QueryJobCreation,
} from "@webui/common/schemas/search";
import {message} from "antd";

import {
    type Aggregation,
    cancelQuery,
    type QueryConfig,
    submitQuery,
} from "../../../../api/search";
import {SETTINGS_STORAGE_ENGINE} from "../../../../config";
import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../SearchState";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import {unquoteString} from "./utils";


/**
 * Builds an API server query config from a query job creation payload.
 *
 * @param payload
 * @param aggregation
 * @return
 */
const buildQueryConfig = (payload: QueryJobCreation, aggregation?: Aggregation): QueryConfig => ({
    // Buffer results in the results cache (MongoDB) so that the results contain structured metadata
    // (e.g., timestamps and original file paths). Also, the `clp` storage engine doesn't support
    // writing search results to files.
    output: {
        type: "results_cache",
        ...("undefined" === typeof aggregation ?
            {} :
            {aggregation}),
        ...("undefined" === typeof payload.maxNumResults ?
            {} :
            {max_num_results: payload.maxNumResults}),
    },
    query: {
        datasets: payload.datasets,
        ignore_case: payload.ignoreCase,
        query_string: payload.queryString,
        time_range: {
            begin_timestamp_millisecs: payload.timestampBegin,
            end_timestamp_millisecs: payload.timestampEnd,
        },
    },
});

/**
 * Submits a new search query to the API server. Two query jobs are submitted: a search job and a
 * count-by-time aggregation job for the results timeline.
 *
 * @param payload
 */
const handleQuerySubmit = (payload: QueryJobCreation) => {
    const store = useSearchStore.getState();

    // User should NOT be able to submit a new query while an existing query is in progress.
    if (
        store.searchUiState !== SEARCH_UI_STATE.DEFAULT &&
        store.searchUiState !== SEARCH_UI_STATE.DONE &&
        store.searchUiState !== SEARCH_UI_STATE.FAILED

    ) {
        console.error("Cannot submit query while existing query is in progress.");

        return;
    }

    if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
        try {
            // Some users wrap their query strings in double quotes (perhaps for clarity or because
            // they think it's required to keep spaces in the query string), so we need to unquote
            // the query string if it's quoted.
            payload.queryString = unquoteString(payload.queryString, '"', "\\");
            if ("" === payload.queryString) {
                message.error("Query string cannot be empty.");

                return;
            }
        } catch (e: unknown) {
            message.error(`Error processing query string: ${e instanceof Error ?
                e.message :
                String(e)}`);

            return;
        }
    }

    store.updateNumSearchResultsTable(SEARCH_STATE_DEFAULT.numSearchResultsTable);
    store.updateNumSearchResultsTimeline(SEARCH_STATE_DEFAULT.numSearchResultsTimeline);
    store.updateNumSearchResultsMetadata(SEARCH_STATE_DEFAULT.numSearchResultsMetadata);
    store.updateSearchUiState(SEARCH_UI_STATE.QUERY_ID_PENDING);

    Promise.all([
        submitQuery(buildQueryConfig(payload)),
        submitQuery(buildQueryConfig(payload, {
            bucket_size_millisecs: payload.timeRangeBucketSizeMillis,
            type: "count_by_time",
        })),
    ])
        .then(([searchJobId, aggregationJobId]) => {
            store.updateSearchJobId(searchJobId.toString());
            store.updateAggregationJobId(aggregationJobId.toString());
            store.updateSearchUiState(SEARCH_UI_STATE.QUERYING);
            store.updateSubmittedMaxNumResults(
                payload.maxNumResults ?? SEARCH_STATE_DEFAULT.submittedMaxNumResults
            );
            console.debug(
                "Search jobs created - ",
                "Search job ID:",
                searchJobId,
                "Aggregation job ID:",
                aggregationJobId
            );
        })
        .catch((err: unknown) => {
            console.error("Failed to submit query:", err);
            store.updateSearchUiState(SEARCH_UI_STATE.FAILED);
        });
};

/**
 * Cancels an ongoing search query on the API server.
 *
 * @param payload
 */
const handleQueryCancel = (payload: QueryJob) => {
    const store = useSearchStore.getState();

    if (store.searchUiState !== SEARCH_UI_STATE.QUERYING) {
        console.error("Cannot cancel query if there is no ongoing query.");

        return;
    }

    store.updateSearchUiState(SEARCH_UI_STATE.DONE);
    Promise.all([
        cancelQuery(payload.searchJobId),
        cancelQuery(payload.aggregationJobId),
    ])
        .then(() => {
            console.debug("Query cancelled successfully");
        })
        .catch((err: unknown) => {
            console.error("Failed to cancel query:", err);
        });
};


export {
    handleQueryCancel,
    handleQuerySubmit,
};
