import type {
    ApiClient,
    components,
} from "./index.js";
import {
    useQueryResultStream,
    type UseQueryResultStreamOptions,
} from "./useQueryResultStream.js";


type TimelineBucket = components["schemas"]["TimelineBucket"];

type UseTimelineResultsOptions = Omit<
    UseQueryResultStreamOptions<TimelineBucket>,
    "fetchResponse" | "parse"
>;

/**
 * Streams timeline buckets for a query job.
 *
 * @param apiClient
 * @param jobId
 * @param options
 * @return The buckets received so far, or `null` before results arrive.
 */
const useTimelineResults = (
    apiClient: ApiClient,
    jobId: string | null,
    options: UseTimelineResultsOptions
): TimelineBucket[] | null => useQueryResultStream(apiClient, jobId, {
    ...options,
    fetchResponse: async (client, id, signal) => {
        // eslint-disable-next-line new-cap
        const {response} = await client.GET("/query_results/{query_job_id}/timeline", {
            headers: {Accept: "text/event-stream"},
            params: {path: {query_job_id: Number(id)}},
            parseAs: "stream",
            signal: signal,
        });

        return response;
    },
    parse: (data) => JSON.parse(data) as TimelineBucket,
});

export type {
    TimelineBucket,
    UseTimelineResultsOptions,
};
export {useTimelineResults};
