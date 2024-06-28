import fastify from "fastify";

import DbManager from "./DbManager.js";
import exampleRoutes from "./routes/examples.js";


/**
 * Creates the Fastify app with the given options.
 *
 * @param {Object} options - The options for creating the Fastify app.
 * @param {import("fastify").FastifyServerOptions} options.fastifyOptions - The Fastify server options.
 * @param {string} options.dbPass - The MySQL database password.
 * @param {string} options.dbUser - The MySQL database user.
 * @return {Promise<import("fastify").FastifyInstance>}
 */
const app = async (options = {}) => {
    console.log(options)
    const server = fastify(options.fastifyOptions);
    await server.register(exampleRoutes);
    await server.register(DbManager, {
        mysqlConfig: {
            host: "127.0.0.1",
            database: "clp-db",
            user: options.dbUser,
            password: options.dbPass,
            port: 3306,
            queryJobsTableName: "query_jobs",
        },
        mongoConfig: {
            host: "127.0.0.1",
            port: 27017,
            database: "clp-search",
            statsCollectionName: "stats",
        },
    });

    return server;
};

export default app;
