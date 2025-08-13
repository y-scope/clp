import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

import {
    CLP_QUERY_ENGINES,
    type SearchResultsMetadataDocument,
} from "../../../../../common/index.js";
import settings from "../../../../settings.json" with {type: "json"};
import {ErrorSchema} from "../../../schemas/error.js";
import {
    PrestoQueryJobCreationSchema,
    PrestoQueryJobSchema,
} from "../../../schemas/presto-search.js";
import {insertPrestoRowsToMongo} from "./utils.js";


/**
 * Presto search API routes.
 *
 * @param fastify
 */
// eslint-disable-next-line max-lines-per-function
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {Presto, mongo} = fastify;
    const mongoDb = mongo.db;

    if ("undefined" === typeof Presto) {
        // If Presto client is not available, skip the plugin registration.
        return;
    }

    if ("undefined" === typeof mongoDb) {
        throw new Error("MongoDB database not found");
    }

    const searchResultsMetadataCollection = mongoDb.collection<SearchResultsMetadataDocument>(
        settings.MongoDbSearchResultsMetadataCollectionName
    );

    /**
     * Submits a search query.
     */
    fastify.post(
        "/query",
        {
            schema: {
                body: PrestoQueryJobCreationSchema,
                response: {
                    [StatusCodes.CREATED]: PrestoQueryJobSchema,
                    [StatusCodes.INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Presto Search"],
            },
        },
        // eslint-disable-next-line max-lines-per-function
        async (request, reply) => {
            const {queryString} = request.body;

            let searchJobId: string;

            try {
                // eslint-disable-next-line max-lines-per-function
                searchJobId = await new Promise<string>((resolve, reject) => {
                    let isResolved = false;
                    Presto.client.execute({
                        // eslint-disable-next-line no-warning-comments
                        // TODO: Error, and success handlers are dummy implementations
                        // and will be replaced with proper implementations.
                        data: (_, data, columns) => {
                            request.log.info(
                                `Received ${data.length} rows from Presto query`
                            );

                            if (false === isResolved) {
                                request.log.error(
                                    "Presto data received before searchJobId was resolved; " +
                                    "skipping insert."
                                );

                                return;
                            }

                            if (0 === data.length) {
                                return;
                            }

                            insertPrestoRowsToMongo(
                                data,
                                columns,
                                searchJobId,
                                mongoDb
                            ).catch((err: unknown) => {
                                request.log.error(
                                    err,
                                    "Failed to insert Presto results into MongoDB"
                                );
                            });
                        },
                        error: (error) => {
                            request.log.info(error, "Presto search failed");
                            if (false === isResolved) {
                                isResolved = true;
                                reject(new Error("Presto search failed"));
                            }
                        },
                        query: queryString,
                        state: (_, queryId, stats) => {
                            request.log.info({
                                searchJobId: queryId,
                                state: stats.state,
                            }, "Presto search state updated");

                            // Insert metadata and resolve queryId on first call
                            if (false === isResolved) {
                                searchResultsMetadataCollection.insertOne({
                                    _id: queryId,
                                    lastSignal: stats.state,
                                    errorMsg: null,
                                    queryEngine: CLP_QUERY_ENGINES.PRESTO,
                                }).catch((err: unknown) => {
                                    request.log.error(err, "Failed to insert Presto metadata");
                                });
                                isResolved = true;
                                resolve(queryId);
                            } else {
                                // Update metadata on subsequent calls
                                searchResultsMetadataCollection.updateOne(
                                    {_id: queryId},
                                    {$set: {lastSignal: stats.state}}
                                ).catch((err: unknown) => {
                                    request.log.error(err, "Failed to update Presto metadata");
                                });
                            }
                        },
                        success: () => {
                            request.log.info("Presto search succeeded");
                        },
                        timeout: null,
                    });
                });
            } catch (error) {
                request.log.error(error, "Failed to submit Presto query");
                throw error;
            }

            reply.code(StatusCodes.CREATED);

            return {searchJobId};
        }
    );

    fastify.post(
        "/cancel",
        {
            schema: {
                body: PrestoQueryJobSchema,
                response: {
                    [StatusCodes.NO_CONTENT]: Type.Null(),
                    [StatusCodes.INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Presto Search"],
            },
        },
        async (request, reply) => {
            const {searchJobId} = request.body;
            await new Promise<void>((resolve, reject) => {
                Presto.client.kill(searchJobId, (error) => {
                    if (null !== error) {
                        reject(new Error("Failed to kill the Presto query job.", {cause: error}));
                    }
                    resolve();
                });
            });
            request.log.info({searchJobId}, "Presto search cancelled");
            reply.code(StatusCodes.NO_CONTENT);

            return null;
        }
    );
};

export default plugin;
