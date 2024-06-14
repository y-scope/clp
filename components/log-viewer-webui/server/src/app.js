import fastify from "fastify";

import exampleRoutes from "./routes/examples.js";


/**
 * Creates the Fastify app with the given options.
 *
 * @param {import("fastify").FastifyServerOptions} fastifyOptions
 * @return {Promise<import("fastify").FastifyInstance>}
 */
const app = async (fastifyOptions = {}) => {
    const server = fastify(fastifyOptions);
    await server.register(exampleRoutes);

    return server;
};

export default app;
