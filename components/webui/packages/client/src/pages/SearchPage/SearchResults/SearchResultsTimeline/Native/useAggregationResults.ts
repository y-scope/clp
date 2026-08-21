import {useQueryResults} from "@webui/api-client/useQueryResults";

import {apiClient} from "../../../../../api/search";
import {TimelineBucket} from "../../../../../components/ResultsTimeline/typings";
import useSearchStore from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";


/**
 * Custom hook to stream aggregation results for the current aggregationJobId from the API
 * server's SSE endpoint.
 *
 * @return
 */
const useAggregationResults = () => {
    const aggregationJobId = useSearchStore((state) => state.aggregationJobId);

    return useQueryResults<TimelineBucket>(apiClient, aggregationJobId, {
        onDone: () => {
            if (null !== aggregationJobId) {
                useSearchStore.getState().markAggregationResultsComplete(aggregationJobId);
            }
        },
        onError: (err) => {
            console.error("Failed to stream aggregation results:", err);
            const {aggregationJobId: currentJobId, searchUiState, updateSearchUiState} =
                useSearchStore.getState();

            if (aggregationJobId === currentJobId && searchUiState === SEARCH_UI_STATE.QUERYING) {
                updateSearchUiState(SEARCH_UI_STATE.FAILED);
            }
        },
        parse: (data) => JSON.parse(data) as TimelineBucket,
        rawDocs: true,
        sorted: false,
    });
};

export {useAggregationResults};
