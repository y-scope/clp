import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

import {
    SEARCH_SIGNAL,
    type SearchResultsMetadataDocument,
} from "../../../../../../common/index.js";
import settings from "../../../../../settings.json" with {type: "json"};
import {
    CLP_DEFAULT_DATASET_NAME,
    CLP_STORAGE_ENGINE_CLP_S,
} from "../../../../configConstants.js";
import {ErrorSchema} from "../../../schemas/error.js";
import {
    QueryJobCreationSchema,
    QueryJobSchema,
} from "../../../schemas/search.js";
import {SEARCH_MAX_NUM_RESULTS} from "./typings.js";
import {
    createMongoIndexes,
    updateSearchResultsMeta,
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
        QueryJobsDbManager,
        mongo,
    } = fastify;
    const mongoDb = mongo.db;

    if ("undefined" === typeof mongoDb) {
        throw new Error("MongoDB database not found");
    }

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
                body: QueryJobCreationSchema,
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
                dataset: CLP_STORAGE_ENGINE_CLP_S === settings.ClpStorageEngine ?
                    CLP_DEFAULT_DATASET_NAME :
                    null,
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
                    mongoDb: mongoDb,
                    queryJobsDbManager: QueryJobsDbManager,
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

            await mongoDb.collection(searchJobId.toString()).drop();
            await mongoDb.collection(aggregationJobId.toString()).drop();

            reply.code(StatusCodes.NO_CONTENT);

            return null;
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
                    searchResultsMetadataCollection: searchResultsMetadataCollection,
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

            reply.code(StatusCodes.NO_CONTENT);

            return null;
        }
    );
};

export default plugin;
