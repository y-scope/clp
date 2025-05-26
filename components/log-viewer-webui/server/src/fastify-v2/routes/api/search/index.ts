import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

import {SEARCH_SIGNAL} from "../../../plugins/app/search/SearchResultsMetadataCollection/typings.js";
import {ErrorSchema} from "../../../schemas/error.js";
import {
    CreateQueryJobSchema,
    QueryJobSchema,
} from "../../../schemas/search.js";
import {SEARCH_MAX_NUM_RESULTS, SearchResultsMetadataDocument} from "./typings.js";
import {
    createMongoIndexes,
    updateSearchResultsMeta,
    updateSearchSignalWhenJobsFinish,
} from "./utils.js";

import settings from "../../../../../settings.json" with {type: "json"};

/**
 * Search API routes.
 *
 * @param fastify
 */
// eslint-disable-next-line max-lines-per-function
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {
        QueryJobsDbManager,
    } = fastify;

    if ("undefined" === typeof fastify.mongo.db) {
        throw new Error("MongoDB database not found");
    }

    const mongoDb = fastify.mongo.db;

    const searchResultsMetadataCollection = mongoDb.collection<SearchResultsMetadataDocument>(
        settings.MongoDbSearchResultsMetadataCollectionName
    );

    /**
     * Submits a search query and initiates the search process.
     */
    fastify.post(
        "/query",
        {
            schema: {
                body: CreateQueryJobSchema,
                response: {
                    [StatusCodes.CREATED]: QueryJobSchema,
                    [StatusCodes.INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Search"],
            },
        },
        async (request, reply) => {
            const {

                timestampBegin,
                timestampEnd,
                ignoreCase,
                timeRangeBucketSizeMillis,
                queryString,
            } = request.body;

            const args = {
                begin_timestamp: timestampBegin,
                end_timestamp: timestampEnd,
                ignore_case: ignoreCase,
                max_num_results: SEARCH_MAX_NUM_RESULTS,
                query_string: queryString,
            };

            request.log.info(args, "/api/search/query args");

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

                return reply.internalServerError(errMsg);
            }

            // Should not throw an error if the collection already exists.
            await mongoDb.createCollection(searchJobId.toString());
            await mongoDb.createCollection(aggregationJobId.toString());

            await searchResultsMetadataCollection.insertOne({
                _id: searchJobId.toString(),
                lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
                errorMsg: null,
            });

            // Defer signal update until after response is sent
            setImmediate(() => {
                updateSearchSignalWhenJobsFinish({
                    aggregationJobId: aggregationJobId,
                    logger: request.log,
                    queryJobsDbManager: QueryJobsDbManager,
                    searchJobId: searchJobId,
                    searchResultsMetadataCollection: SearchResultsMetadataCollection,
                }).catch((err: unknown) => {
                    request.log.error(err, "Error updating search signal when jobs finish");
                });
            });

            await createMongoIndexes({
                logger: request.log,
                searchJobId: searchJobId,
                searchJobCollectionsManager: SearchJobCollectionsManager,
            });

            reply.code(StatusCodes.CREATED);

            return {searchJobId, aggregationJobId};
        }
    );

    /**
     * Clears the results of a search operation identified by jobId.
     */
    fastify.delete(
        "/results",
        {
            schema: {
                body: QueryJobSchema,
                response: {
                    [StatusCodes.NO_CONTENT]: Type.Null(),
                    [StatusCodes.INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Search"],
            },
        },
        async (request, reply) => {
            const {searchJobId, aggregationJobId} = request.body;

            request.log.info({
                searchJobId,
                aggregationJobId,
            }, "api/search/results args");

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

                return reply.internalServerError(`${errMsg} for searchJobId=${searchJobId}, ` +
                `aggregationJobId=${aggregationJobId}`);
            }

            return reply.code(StatusCodes.NO_CONTENT);
        }
    );


    fastify.post(
        "/cancel",
        {
            schema: {
                body: QueryJobSchema,
                response: {
                    [StatusCodes.NO_CONTENT]: Type.Null(),
                    [StatusCodes.INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Search"],
            },
        },
        async (request, reply) => {
            const {searchJobId, aggregationJobId} = request.body;

            request.log.info({
                searchJobId,
                aggregationJobId,
            }, "api/search/cancel args");

            try {
                await QueryJobsDbManager.submitQueryCancellation(searchJobId);
                await QueryJobsDbManager.submitQueryCancellation(aggregationJobId);

                await updateSearchResultsMeta({
                    fields: {
                        lastSignal: SEARCH_SIGNAL.RESP_DONE,
                        errorMsg: "Query cancelled before it could be completed.",
                    },
                    jobId: searchJobId,
                    lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
                    logger: request.log,
                    searchResultsMetadataCollection: SearchResultsMetadataCollection,
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

                return reply.internalServerError(`${errMsg} for searchJobId=${searchJobId}, ` +
                `aggregationJobId=${aggregationJobId}`);
            }

            return reply.code(StatusCodes.NO_CONTENT);
        }
    );
};

export default plugin;
