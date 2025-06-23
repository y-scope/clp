import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

import {SqlSchema} from "../../../schemas/archive-metadata.js";


/**
 * Archive metadata API routes.
 *
 * @param fastify
 */
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
            const [result] = await mysqlConnectionPool.query(queryString);
            reply.code(StatusCodes.OK);

            return result;
        },
    );
};

export default plugin;
