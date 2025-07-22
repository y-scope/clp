import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";
import {Client} from "presto-client";

import settings from "../../../../settings.json" with {type: "json"};
import {ErrorSchema} from "../../../schemas/error.js";
import {
    PrestoJobSchema,
    PrestoSearchJobCreationSchema,
} from "../../../schemas/presto-search.js";
import {Nullable} from "../../../typings/common.js";


/**
 * Presto search API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    if (false === settings.EnablePresto) {
        return;
    }

    const client = new Client({host: settings.PrestoHost, port: settings.PrestoPort});

    /**
     * Submits a search query and initiates the search process.
     */
    fastify.post(
        "/query",
        {
            schema: {
                body: PrestoSearchJobCreationSchema,
                response: {
                    [StatusCodes.CREATED]: PrestoJobSchema,
                    [StatusCodes.INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Search"],
            },
        },

        async (request, reply) => {
            const {queryString} = request.body;

            let searchJobId: Nullable<string> = null;

            searchJobId = await new Promise((resolve) => {
                client.execute({
                    data: (error, data, columns) => {
                        if (null === searchJobId) {
                            request.log.error("Data arrived from Presto before the search job id.");
                        }
                        request.log.info({error, searchJobId, columns, data}, "Presto data");
                    },
                    error: (error) => {
                        request.log.info(error, "Presto search failed");

                        // The `/query` endpoint will catch this error if `resolve` hasn't
                        // been called.
                        throw new Error("Presto search failed.");
                    },
                    query: queryString,
                    state: (error, newSearchJobId, stats) => {
                        request.log.info({
                            error: error,
                            searchJobId: newSearchJobId,
                            state: stats.state,
                        }, "Presto search state updated");

                        resolve(newSearchJobId);
                    },
                    success: () => {
                        request.log.info("Presto search succeeded");
                    },
                });
            });

            reply.code(StatusCodes.CREATED);

            if (null === searchJobId) {
                throw new Error("searchJobId is null");
            }

            return {searchJobId};
        }
    );
};

export default plugin;
