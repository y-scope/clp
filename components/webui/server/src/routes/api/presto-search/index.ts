import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

import {
    SEARCH_SIGNAL,
    type SearchResultsMetadataDocument,
} from "../../../../../common/index.js";
import settings from "../../../../settings.json" with {type: "json"};
import {ErrorSchema} from "../../../schemas/error.js";
import {
    PrestoQueryJobCreationSchema,
    PrestoQueryJobSchema,
} from "../../../schemas/presto-search.js";


/**
 * Presto search API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {Presto, mongo} = fastify;

    if ("undefined" === typeof Presto) {
        // If Presto client is not available, skip the plugin registration.
        return;
    }

    if ("undefined" === typeof mongo) {
        throw new Error("MongoDB not found");
    }

    const mongoDb = mongo.db;
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

        async (request, reply) => {
            const {queryString} = request.body;

            let searchJobId: string;

            try {
                searchJobId = await new Promise<string>((resolve, reject) => {
                    let isResolved = false;
                    let resultCount = 0;

                    Presto.client.execute({
                        data: (_, data, columns) => {
                            request.log.info({columns, dataLength: data.length}, "Presto data received");

                            // Store data in MongoDB collection for this search job
                            if (data && data.length > 0) {
                                const collection = mongoDb.collection(searchJobId);
                                const documents = data.map((row) => ({
                                    _id: resultCount++,
                                    timestamp: Date.now(),
                                    message: row,
                                    columns: columns,
                                }));

                                collection.insertMany(documents).catch((err: unknown) => {
                                    request.log.error(err, "Failed to insert Presto data into MongoDB");
                                });
                            }
                        },
                        error: (error) => {
                            request.log.error(error, "Presto search failed");
                            if (false === isResolved) {
                                isResolved = true;

                                // Update metadata with error
                                searchResultsMetadataCollection.updateOne(
                                    {_id: searchJobId},
                                    {
                                        $set: {
                                            lastSignal: SEARCH_SIGNAL.RESP_DONE,
                                            errorMsg: String(error),
                                        }
                                    }
                                ).catch((err: unknown) => {
                                    request.log.error(err, "Failed to update error metadata");
                                });

                                reject(new Error("Presto search failed"));
                            }
                        },
                        query: queryString,
                        state: (_, queryId, stats) => {
                            request.log.info({
                                searchJobId: queryId,
                                state: stats.state,
                            }, "Presto search state updated");

                            if (false === isResolved) {
                                isResolved = true;
                                resolve(queryId);
                            }
                        },
                        success: () => {
                            request.log.info("Presto search succeeded");

                            // Update metadata on success
                            searchResultsMetadataCollection.updateOne(
                                {_id: searchJobId},
                                {
                                    $set: {
                                        lastSignal: SEARCH_SIGNAL.RESP_DONE,
                                        errorMsg: null,
                                    }
                                }
                            ).catch((err: unknown) => {
                                request.log.error(err, "Failed to update success metadata");
                            });
                        },
                    });
                });

                // Create MongoDB collection for storing query results
                await mongoDb.createCollection(searchJobId);

                // Insert initial metadata
                await searchResultsMetadataCollection.insertOne({
                    _id: searchJobId,
                    lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
                    errorMsg: null,
                });

            } catch (error) {
                request.log.error(error, "Failed to submit Presto query");
                throw error;
            }

            reply.code(StatusCodes.CREATED);

            return {searchJobId};
        }
    );
};

export default plugin;
