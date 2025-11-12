import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {CLP_QUERY_ENGINES} from "@webui/common/config";
import {
    SEARCH_SIGNAL,
    type SearchResultsMetadataDocument,
} from "@webui/common/metadata";
import {QUERY_JOB_TYPE} from "@webui/common/query";
import {ErrorSchema} from "@webui/common/schemas/error";
import {
    QueryJobCreationSchema,
    QueryJobSchema,
} from "@webui/common/schemas/search";
import {constants} from "http2";

import settings from "../../../../settings.json" with {type: "json"};
import {SEARCH_MAX_NUM_RESULTS} from "./typings.js";
import {
    createMongoIndexes,
    updateSearchSignalWhenJobsFinish,
} from "./utils.js";


/**
 * Search API routes.
 *
 * @param fastify
 */
// eslint-disable-next-line max-lines-per-function
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {
        QueryJobDbManager,
        mongo,
    } = fastify;
    const mongoDb = mongo.db;

    if ("undefined" === typeof mongoDb) {
        throw new Error("MongoDB database not found");
    }

    const searchResultsMetadataCollection = mongoDb.collection<SearchResultsMetadataDocument>(
        settings.MongoDbSearchResultsMetadataCollectionName
    );

    const queryEngine = settings.ClpQueryEngine as CLP_QUERY_ENGINES;

    /**
     * Submits a search query and initiates the search process.
     */
    fastify.post(
        "/query",
        {
            schema: {
                body: QueryJobCreationSchema,
                response: {
                    [constants.HTTP_STATUS_CREATED]: QueryJobSchema,
                    [constants.HTTP_STATUS_INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Search"],
            },
        },
        // eslint-disable-next-line max-lines-per-function
        async (request, reply) => {
            const {
                dataset,
                timestampBegin,
                timestampEnd,
                ignoreCase,
                timeRangeBucketSizeMillis,
                queryString,
            } = request.body;

            const args = {
                begin_timestamp: timestampBegin,
                dataset: dataset,
                end_timestamp: timestampEnd,
                ignore_case: ignoreCase,
                max_num_results: SEARCH_MAX_NUM_RESULTS,
                query_string: queryString,
            };

            request.log.info(args, "/api/search/query args");

            let searchJobId: number;
            let aggregationJobId: number;

            try {
                searchJobId = await QueryJobDbManager.submitJob(
                    args,
                    QUERY_JOB_TYPE.SEARCH_OR_AGGREGATION
                );

                aggregationJobId = await QueryJobDbManager.submitJob(
                    {
                        ...args,
                        aggregation_config: {
                            count_by_time_bucket_size: timeRangeBucketSizeMillis,
                        },
                    },
                    QUERY_JOB_TYPE.SEARCH_OR_AGGREGATION
                );
            } catch (err: unknown) {
                const errMsg = "Unable to submit search/aggregation job to the SQL database";
                request.log.error(err, errMsg);

                return reply.internalServerError(errMsg);
            }

            await mongoDb.createCollection(searchJobId.toString());
            await mongoDb.createCollection(aggregationJobId.toString());

            await searchResultsMetadataCollection.insertOne({
                _id: searchJobId.toString(),
                errorMsg: null,
                errorName: null,
                lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
                queryEngine: queryEngine,
            });

            // Defer signal update until after response is sent
            setImmediate(() => {
                updateSearchSignalWhenJobsFinish({
                    aggregationJobId: aggregationJobId,
                    logger: request.log,
                    mongoDb: mongoDb,
                    queryJobDbManager: QueryJobDbManager,
                    searchJobId: searchJobId,
                    searchResultsMetadataCollection: searchResultsMetadataCollection,
                }).catch((err: unknown) => {
                    request.log.error(err, "Error updating search signal when jobs finish");
                });
            });

            await createMongoIndexes({
                searchJobId: searchJobId,
                logger: request.log,
                mongoDb: mongoDb,
            });
            reply.code(constants.HTTP_STATUS_CREATED);

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
                    [constants.HTTP_STATUS_NO_CONTENT]: Type.Null(),
                    [constants.HTTP_STATUS_INTERNAL_SERVER_ERROR]: ErrorSchema,
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

            await mongoDb.collection(searchJobId.toString()).drop();
            await mongoDb.collection(aggregationJobId.toString()).drop();

            reply.code(constants.HTTP_STATUS_NO_CONTENT);

            return null;
        }
    );


    fastify.post(
        "/cancel",
        {
            schema: {
                body: QueryJobSchema,
                response: {
                    [constants.HTTP_STATUS_NO_CONTENT]: Type.Null(),
                    [constants.HTTP_STATUS_INTERNAL_SERVER_ERROR]: ErrorSchema,
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
                await QueryJobDbManager.cancelJob(searchJobId);
                await QueryJobDbManager.cancelJob(aggregationJobId);

                await searchResultsMetadataCollection.updateOne(
                    {
                        _id: searchJobId.toString(),
                        lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
                    },
                    {
                        $set: {
                            errorMsg: "Query cancelled before it could be completed.",
                            lastSignal: SEARCH_SIGNAL.RESP_DONE,
                        },
                    }
                );
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

            reply.code(constants.HTTP_STATUS_NO_CONTENT);

            return null;
        }
    );
};

export default plugin;
