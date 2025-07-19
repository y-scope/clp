import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {Type} from "@sinclair/typebox";


/**
 * Creates example routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    fastify.get("/example/get/:name", {
        schema: {
            params: Type.Object({
                name: Type.String(),
            }),
        },
    }, async (req) => {
        return {msg: `Hello, ${req.params.name}!`};
    });

    fastify.post("/example/post", {
        schema: {
            body: Type.Object({
                name: Type.String(),
            }),
        },
    }, async (req) => {
        return {msg: `Goodbye, ${req.body.name}!`};
    });
};

export default plugin;
