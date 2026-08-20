import {
    useEffect,
    useRef,
    useState,
} from "react";

import {EventSourceParserStream} from "eventsource-parser/stream";

import type {ApiClient} from "./index.js";


/**
 * Minimum delay between incremental result updates while a stream is in progress. Batches
 * bursts of SSE events into a single state update to avoid excessive re-renders.
 */
const RESULTS_FLUSH_INTERVAL_MILLIS = 100;

/**
 * Options for `useQueryResults`.
 */
interface UseQueryResultsOptions<T> {
    /**
     * Parses a single SSE event payload into a result item.
     */
    parse: (data: string) => T;

    /**
     * Comparator applied to the results before they're returned.
     */
    compareFn?: (a: T, b: T) => number;

    /**
     * Called when the stream ends normally (i.e., all results have been received).
     */
    onDone?: () => void;

    /**
     * Called when the stream fails. Not called when the stream is aborted (e.g., on unmount or
     * when `jobId` changes).
     */
    onError?: (err: unknown) => void;

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
 * Custom hook to reactively stream the results of a query job from the API server's Server-Sent
 * Events (SSE) endpoint. Whenever `jobId` changes, any in-flight stream is aborted and a new one
 * is started.
 *
 * @param apiClient The API server client.
 * @param jobId The query job ID, or `null` if there is no active job.
 * @param options
 * @return The results received so far, updated incrementally (throttled) while the stream is in
 * progress, or `null` while there is no active job or no results have been received yet.
 */
// eslint-disable-next-line max-lines-per-function
const useQueryResults = <T>(
    apiClient: ApiClient,
    jobId: string | null,
    options: UseQueryResultsOptions<T>
): T[] | null => {
    const [results, setResults] = useState<T[] | null>(null);

    // Keep the latest options in a ref so that the stream isn't restarted when callers pass
    // inline callbacks.
    const optionsRef = useRef<UseQueryResultsOptions<T>>(options);
    useEffect(() => {
        optionsRef.current = options;
    });

    // eslint-disable-next-line max-lines-per-function
    useEffect(() => {
        setResults(null);

        if (null === jobId) {
            return () => {
            };
        }

        console.log(`Streaming query results for job ID: ${jobId}`);

        const abortController = new AbortController();
        const collected: T[] = [];
        let flushTimeout: ReturnType<typeof setTimeout> | null = null;

        const flush = () => {
            if (null !== flushTimeout) {
                clearTimeout(flushTimeout);
                flushTimeout = null;
            }

            const snapshot = [...collected];
            const {compareFn} = optionsRef.current;
            if ("undefined" !== typeof compareFn) {
                snapshot.sort(compareFn);
            }
            setResults(snapshot);
        };

        const streamResults = async (): Promise<void> => {
            // eslint-disable-next-line new-cap
            const {response} = await apiClient.GET("/query_results/{search_job_id}", {
                headers: {Accept: "text/event-stream"},
                params: {
                    path: {search_job_id: Number(jobId)},
                    query: {
                        raw_docs: optionsRef.current.rawDocs ?? false,
                        sorted: optionsRef.current.sorted ?? false,
                    },
                },
                parseAs: "stream",
                signal: abortController.signal,
            });

            if (false === response.ok || null === response.body) {
                throw new Error(
                    `Failed to fetch results for query job ${jobId}: HTTP ${response.status}`
                );
            }

            const reader = response.body
                .pipeThrough(new TextDecoderStream())
                .pipeThrough(new EventSourceParserStream())
                .getReader();

            for (
                let chunk = await reader.read();
                false === chunk.done;
                chunk = await reader.read()
            ) {
                collected.push(optionsRef.current.parse(chunk.value.data));
                flushTimeout ??= setTimeout(flush, RESULTS_FLUSH_INTERVAL_MILLIS);
            }
        };

        streamResults()
            .then(() => {
                flush();
                optionsRef.current.onDone?.();
            })
            .catch((err: unknown) => {
                if (abortController.signal.aborted) {
                    return;
                }
                optionsRef.current.onError?.(err);
            });

        return () => {
            abortController.abort();
            if (null !== flushTimeout) {
                clearTimeout(flushTimeout);
            }
        };
    }, [
        apiClient,
        jobId,
    ]);

    return results;
};

export type {UseQueryResultsOptions};
export {useQueryResults};
