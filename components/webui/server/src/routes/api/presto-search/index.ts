import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

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
                    // eslint-disable-next-line no-warning-comments
                    // TODO: Data, error, and success handlers are dummy implementations
                    // and will be replaced with proper implementations.
                    Presto.client.execute({
                        data: (_, data, columns) => {
                            request.log.info(
                                `Received ${data.length} rows from Presto query`
                            );

                            insertPrestoRowsToMongo({
                                columns: columns,
                                data: data,
                                isResolved: isResolved,
                                log: request.log,
                                mongoDb: mongoDb,
                                searchJobId: searchJobId,
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

                            if (false === isResolved) {
                                isResolved = true;
                                resolve(queryId);
                            }
                        },
                        success: () => {
                            request.log.info("Presto search succeeded");
                        },
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
};

export default plugin;
