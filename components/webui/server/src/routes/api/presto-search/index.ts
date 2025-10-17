import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {CLP_QUERY_ENGINES} from "@webui/common/config";
import {
    PRESTO_SEARCH_SIGNAL,
    type SearchResultsMetadataDocument,
} from "@webui/common/metadata";
import {ErrorSchema} from "@webui/common/schemas/error";
import {
    PrestoQueryJobCreationSchema,
    PrestoQueryJobSchema,
} from "@webui/common/schemas/presto-search";
import {constants} from "http2";

import settings from "../../../../settings.json" with {type: "json"};
import {MAX_PRESTO_SEARCH_RESULTS} from "./typings.js";
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
                    [constants.HTTP_STATUS_CREATED]: PrestoQueryJobSchema,
                    [constants.HTTP_STATUS_INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Presto Search"],
            },
        },
        // eslint-disable-next-line max-lines-per-function
        async (request, reply) => {
            const {queryString} = request.body;

            let searchJobId: string;
            let totalResultsCount = 0;
            let storedResultsCount = 0;

            try {
                // eslint-disable-next-line max-lines-per-function
                searchJobId = await new Promise<string>((resolve, reject) => {
                    let isResolved = false;
                    Presto.client.execute({
                        data: (_, data, columns) => {
                            totalResultsCount += data.length;

                            request.log.info(
                                `Received ${data.length} rows from Presto query ` +
                                `(total: ${totalResultsCount})`
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

                            if (storedResultsCount < MAX_PRESTO_SEARCH_RESULTS) {
                                const remainingSlots =
                                    MAX_PRESTO_SEARCH_RESULTS - storedResultsCount;
                                const dataToInsert = data.slice(0, remainingSlots);

                                if (0 < dataToInsert.length) {
                                    storedResultsCount += dataToInsert.length;
                                    insertPrestoRowsToMongo(
                                        dataToInsert,
                                        columns,
                                        searchJobId,
                                        mongoDb
                                    ).catch((err: unknown) => {
                                        request.log.error(
                                            err,
                                            "Failed to insert Presto results into MongoDB"
                                        );
                                    });
                                }
                            }

                            // Always update metadata with total count
                            searchResultsMetadataCollection.updateOne(
                                {_id: searchJobId},
                                {$set: {numTotalResults: totalResultsCount}}
                            ).catch((err: unknown) => {
                                request.log.error(
                                    err,
                                    "Failed to update total results count in metadata"
                                );
                            });
                        },
                        error: (error) => {
                            request.log.info(error, "Presto search failed");
                            if (false === isResolved) {
                                isResolved = true;
                                reject(new Error("Presto search failed"));
                            } else {
                                searchResultsMetadataCollection.updateOne(
                                    {_id: searchJobId},
                                    {
                                        $set: {
                                            errorMsg: error.message,
                                            errorName: ("errorName" in error) ?
                                                error.errorName :
                                                null,
                                            lastSignal: PRESTO_SEARCH_SIGNAL.FAILED,
                                        },
                                    }
                                ).catch((err: unknown) => {
                                    request.log.error(
                                        err,
                                        "Failed to update Presto error metadata"
                                    );
                                });
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
                                    errorMsg: null,
                                    errorName: null,
                                    lastSignal: PRESTO_SEARCH_SIGNAL.QUERYING,
                                    queryEngine: CLP_QUERY_ENGINES.PRESTO,
                                }).catch((err: unknown) => {
                                    request.log.error(err, "Failed to insert Presto metadata");
                                });
                                isResolved = true;
                                resolve(queryId);
                            }
                        },
                        success: () => {
                            if (false === isResolved) {
                                request.log.error(
                                    "Presto query finished before searchJobId was resolved; "
                                );

                                return;
                            }
                            searchResultsMetadataCollection.updateOne(
                                {_id: searchJobId},
                                {$set: {lastSignal: PRESTO_SEARCH_SIGNAL.DONE}}
                            ).catch((err: unknown) => {
                                request.log.error(err, "Failed to update Presto metadata");
                            });

                            request.log.info("Presto search succeeded");
                        },
                        timeout: null,
                    });
                });
            } catch (error) {
                request.log.error(error, "Failed to submit Presto query");
                throw error;
            }

            await mongoDb.createCollection(searchJobId);

            reply.code(constants.HTTP_STATUS_CREATED);

            return {searchJobId};
        }
    );

    fastify.post(
        "/cancel",
        {
            schema: {
                body: PrestoQueryJobSchema,
                response: {
                    [constants.HTTP_STATUS_NO_CONTENT]: Type.Null(),
                    [constants.HTTP_STATUS_INTERNAL_SERVER_ERROR]: ErrorSchema,
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
            reply.code(constants.HTTP_STATUS_NO_CONTENT);

            return null;
        }
    );

    fastify.delete(
        "/results",
        {
            schema: {
                body: PrestoQueryJobSchema,
                response: {
                    [constants.HTTP_STATUS_NO_CONTENT]: Type.Null(),
                    [constants.HTTP_STATUS_INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Presto Search"],
            },
        },
        async (request, reply) => {
            const {searchJobId} = request.body;

            request.log.info({
                searchJobId,
            }, "api/presto-search/results args");

            await mongoDb.collection(searchJobId).drop();

            reply.code(constants.HTTP_STATUS_NO_CONTENT);

            return null;
        }
    );
};

export default plugin;
