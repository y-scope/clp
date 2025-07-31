import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

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
    const {Presto} = fastify;

    if ("undefined" === typeof Presto) {
        throw new Error("Presto not available");
    }

    /**
     * Submits a search query and initiates the search process.
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

                    Presto.client.execute({
                        // eslint-disable-next-line no-warning-comments
                        // TODO: Data, error, and success handlers are dummy implementations
                        // and should be completed.
                        data: (_, data, columns) => {
                            request.log.info({columns, data}, "Presto data");
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
