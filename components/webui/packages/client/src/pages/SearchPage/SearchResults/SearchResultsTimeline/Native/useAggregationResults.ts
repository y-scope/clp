import {useTimelineResults} from "@webui/api-client/useTimelineResults";

import {apiClient} from "../../../../../api/search";
import useSearchStore from "../../../SearchState/index";


/**
 * Custom hook to stream aggregation results for the current aggregationJobId from the API
 * server's SSE endpoint.
 *
 * @return
 */
const useAggregationResults = () => {
    const aggregationJobId = useSearchStore((state) => state.aggregationJobId);

    return useTimelineResults(apiClient, aggregationJobId, {
        onError: (err) => {
            console.error("Failed to stream aggregation results:", err);
        },
    });
};

export {useAggregationResults};
