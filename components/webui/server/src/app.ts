import {
    FastifyInstance,
    FastifyPluginAsync,
} from "fastify";

import staticRoutes from "./routes/static.js";


/**
 * Creates the Fastify app with the given options.
 *
 * TODO: Once old webui code is refactored to new modlular fastify style, this plugin should be
 * removed.
 *
 * @param fastify
 * @return
 */
const FastifyV1App: FastifyPluginAsync = async (
    fastify: FastifyInstance
) => {
    // Register the routes
    await fastify.register(staticRoutes);
};

export default FastifyV1App;
