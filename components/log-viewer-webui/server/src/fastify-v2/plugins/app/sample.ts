import fp from "fastify-plugin";


/**
 * Example to demonstrate how to create a Fastify plugin.
 *
 * TODO: Remove example code when new webui app code is ready.
 *
 * @return plugin interface.
 */
const createExamplePlugin = () => {
    return {
        /**
         * Returns `Example`.
         *
         * @return
         */
        async getExample (): Promise<string> {
            return "Example";
        },
    };
};

export default fp(async (fastify) => {
    fastify.decorate("ExamplePlugin", createExamplePlugin());
}, {
    name: "ExamplePlugin",
});

declare module "fastify" {
    export interface FastifyInstance {
        ExamplePlugin: ReturnType<typeof createExamplePlugin>;
    }
}
