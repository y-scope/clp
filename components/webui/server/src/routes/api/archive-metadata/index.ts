import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {SqlSchema} from "@webui/common/schemas/archive-metadata";
import {constants} from "http2";


/**
 * Archive metadata API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    fastify.post(
        "/sql",
        {
            schema: {
                body: SqlSchema,
                response: {
                    [constants.HTTP_STATUS_OK]: Type.Any(),
                },
                tags: ["Archive Metadata"],
            },
        },
        async (req, reply) => {
            const {queryString} = req.body;
            const [result] = await fastify.mysql.query(queryString);
            reply.code(constants.HTTP_STATUS_OK);

            return result;
        },
    );
};

export default plugin;
