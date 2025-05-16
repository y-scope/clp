import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";

import {SEARCH_SIGNAL} from "../../../plugins/app/search/SearchResultsMetadataCollection/typings.js";
import {ErrorSchema} from "../../../schemas/error.js";
import {
    CreateSearchJobSchema,
    SearchJobSchema,
} from "../../../schemas/search.js";
import {SEARCH_MAX_NUM_RESULTS} from "./typings.js";
import {
    createMongoIndexes,
    updateSearchResultsMeta,
    updateSearchSignalWhenJobsFinish,
} from "./utils.js";


/**
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {
        QueryJobsDbManager,
        SearchJobCollectionsManager,
        SearchResultsMetadataCollection,
    } = fastify;

    /**
     * Submits a search query and initiates the search process.
     */
    fastify.post(
        "/query",
        {
            schema: {
                body: CreateSearchJobSchema,
                response: {
                    201: SearchJobSchema,
                    500: ErrorSchema,
                },
                tags: ["Search"],
            },
        },
        async (request, reply) => {
            const {
                queryString,
                timestampBegin,
                timestampEnd,
                ignoreCase,
                timeRangeBucketSizeMillis,
            } = request.body;

            const args = {
                query_string: queryString,
                begin_timestamp: timestampBegin,
                end_timestamp: timestampEnd,
                ignore_case: ignoreCase,
                max_num_results: SEARCH_MAX_NUM_RESULTS,
            };

            request.log.info({args}, "/search args");

            let searchJobId: number;
            let aggregationJobId: number;

            try {
                searchJobId = await QueryJobsDbManager.submitSearchJob(args);
                aggregationJobId = await QueryJobsDbManager.submitAggregationJob(
                    args,
                    timeRangeBucketSizeMillis
                );
            } catch (err: unknown) {
                const errMsg = "Unable to submit search/aggregation job to the SQL database";
                request.log.error(err, errMsg);
                reply.code(500);

                return {message: errMsg};
            }

            SearchResultsMetadataCollection.insertOne({
                _id: searchJobId.toString(),
                lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
                errorMsg: null,
            });

            // Defer signal update until after response is sent
            setImmediate(async () => {
                await updateSearchSignalWhenJobsFinish({
                    searchJobId,
                    aggregationJobId,
                    queryJobsDbManager: QueryJobsDbManager,
                    searchJobCollectionsManager: SearchJobCollectionsManager,
                    SearchResultsMetadataCollection,
                    logger: request.log,
                });
            });

            await createMongoIndexes({
                searchJobId,
                searchJobCollectionsManager: SearchJobCollectionsManager,
                logger: request.log,
            });

            reply.code(201);

            return {searchJobId, aggregationJobId};
        }
    );

    /**
     * Clears the results of a search operation identified by jobId.
     */
    fastify.delete(
        "/search/results",
        {
            schema: {
                body: SearchJobSchema,
                response: {
                    204: Type.Null(),
                    500: ErrorSchema,
                },
                tags: ["Search"],
            },
        },
        async (request, reply) => {
            const {searchJobId, aggregationJobId} = request.body;

            request.log.info({
                searchJobId,
                aggregationJobId,
            }, "/search/results args");

            try {
                await SearchJobCollectionsManager.dropCollection(searchJobId);
                await SearchJobCollectionsManager.dropCollection(aggregationJobId);
            } catch (err: unknown) {
                const errMsg = "Failed to clear search results";
                request.log.error(
                    {
                        err,
                        searchJobId,
                        aggregationJobId,
                    },
                    errMsg
                );
                reply.code(500);

                return {message: `${errMsg} for searchJobId=${searchJobId}, ` +
                `aggregationJobId=${aggregationJobId}`};
            }
            reply.code(204);
        }
    );


    fastify.post(
        "/search/cancel",
        {
            schema: {
                body: SearchJobSchema,
                response: {
                    204: Type.Null(),
                    500: ErrorSchema,
                },
                tags: ["Search"],
            },
        },
        async (request, reply) => {
            const {searchJobId, aggregationJobId} = request.body;

            request.log.info(
                `/search/cancel searchJobId=${searchJobId}, aggregationJobId=${aggregationJobId}`
            );

            try {
                await QueryJobsDbManager.submitQueryCancellation(searchJobId);
                await QueryJobsDbManager.submitQueryCancellation(aggregationJobId);

                updateSearchResultsMeta({
                    jobId: searchJobId,
                    lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
                    SearchResultsMetadataCollection,
                    logger: request.log,
                    fields: {
                        lastSignal: SEARCH_SIGNAL.RESP_DONE,
                        errorMsg: "Query cancelled before it could be completed.",
                    },
                });
            } catch (err: unknown) {
                const errMsg = "Failed to submit cancel request";
                request.log.error(
                    {
                        err,
                        searchJobId,
                        aggregationJobId,
                    },
                    errMsg
                );
                reply.code(500);

                return {message: `${errMsg} for searchJobId=${searchJobId}, ` +
                `aggregationJobId=${aggregationJobId}`};
            }

            reply.code(204);
        }
    );
};

export default plugin;
