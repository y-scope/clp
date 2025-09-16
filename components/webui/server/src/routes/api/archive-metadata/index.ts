import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {SqlSchema} from "@webui/common/schemas/archive-metadata";
import {StatusCodes} from "http-status-codes";


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
