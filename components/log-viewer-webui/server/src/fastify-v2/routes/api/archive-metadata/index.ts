import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

import {SqlSchema} from "../../../schemas/archive-metadata.js";
/**
 * Search API routes.
 *
 * @param fastify
 */
// eslint-disable-next-line max-lines-per-function
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const mysqlConnectionPool = fastify.mysql.pool;

    fastify.post(
        "/sql",
        {
            schema: {
                body: SqlSchema,
                response: {
                    [StatusCodes.OK]: Type.Any(),
                },
                tags: ["Archive Metadata"],
            },
        },
        async (req, reply) => {
            const {queryString} = req.body;
            reply.code(StatusCodes.OK);
            return await mysqlConnectionPool.query(queryString);
        },
    );
};

export default plugin;
