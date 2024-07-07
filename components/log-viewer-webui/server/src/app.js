import fastify from "fastify";
import * as path from "node:path";
import process from "node:process";

import {fastifyStatic} from "@fastify/static";

import exampleRoutes from "./routes/examples.js";


/**
 * Creates the Fastify app with the given options.
 *
 * @param {string} clientDir Absolute path to the client directory to serve when in running in a
 * production environment.
 * @param {import("fastify").FastifyServerOptions} fastifyOptions
 * @return {Promise<import("fastify").FastifyInstance>}
 */
const app = async (clientDir, fastifyOptions = {}) => {
    const server = fastify(fastifyOptions);

    if ("production" === process.env.NODE_ENV) {
        // In the development environment, we expect the client to use a separate webserver that
        // supports live reloading.
        if (false === path.isAbsolute(clientDir)) {
            throw new Error("`clientDir` must be an absolute path.");
        }

        await server.register(fastifyStatic, {
            prefix: "/",
            root: clientDir,
        });
    }
    await server.register(exampleRoutes);

    return server;
};

export default app;
