import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";


/**
 * Example to demonstrate how to create a Fastify route.
 *
 * TODO: Remove example code when new webui app code is ready.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {ExamplePlugin} = fastify;
    fastify.get(
        "/Example",
        {
            schema: {
                response: {
                    200: Type.Object({
                        message: Type.String(),
                    }),
                },
            },
        },
        async () => {
            const exampleMessage = await ExamplePlugin.getExample();
            return {message: exampleMessage};
        }
    );
};

export default plugin;
