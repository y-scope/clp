import type {ApiClient} from "./index.js";
import {
    useQueryResultStream,
    type UseQueryResultStreamOptions,
} from "./useQueryResultStream.js";


/**
 * Options for `useQueryResults`.
 */
interface UseQueryResultsOptions<T> extends Omit<UseQueryResultStreamOptions<T>, "fetchResponse"> {
    /**
     * Whether to request raw result documents (including metadata) rather than just log
     * messages. Only applies to query jobs whose results are buffered in MongoDB.
     */
    rawDocs?: boolean;

    /**
     * Whether to request results sorted by timestamp descending from the API server. Only
     * applies to query jobs whose results are buffered in MongoDB. When `false`, the server
     * streams results in insertion order and any `compareFn` is still applied client-side.
     */
    sorted?: boolean;
}

/**
 * Custom hook to reactively stream search results from the API server's Server-Sent Events
 * (SSE) endpoint. Whenever `jobId` changes, any in-flight stream is aborted and a new one
 * is started.
 *
 * @param apiClient The API server client.
 * @param jobId The query job ID, or `null` if there is no active job.
 * @param options
 * @return The results received so far, updated incrementally (throttled) while the stream is in
 * progress, or `null` while there is no active job or no results have been received yet.
 */
const useQueryResults = <T>(
    apiClient: ApiClient,
    jobId: string | null,
    options: UseQueryResultsOptions<T>
): T[] | null => useQueryResultStream(apiClient, jobId, {
    ...options,
    fetchResponse: async (client, id, signal) => {
        // eslint-disable-next-line new-cap
        const {response} = await client.GET("/query_results/{query_job_id}", {
            headers: {Accept: "text/event-stream"},
            params: {
                path: {query_job_id: Number(id)},
                query: {
                    raw_docs: options.rawDocs ?? false,
                    sorted: options.sorted ?? false,
                },
            },
            parseAs: "stream",
            signal: signal,
        });

        return response;
    },
});

export type {UseQueryResultsOptions};
export {useQueryResults};
