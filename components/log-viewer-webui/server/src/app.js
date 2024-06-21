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
        mysqlDbHost: "localhost",
        mysqlDbName: "clp-db",
        mysqlDbPassword: options.dbPass,
        mysqlDbPort: 3306,
        mysqlDbUser: options.dbUser,
        mysqlQueryJobsTableName: "query_jobs",
        mongoStatsCollectionName: "stats",
        mongoDbHost: "localhost",
        mongoDbName: "clp-search",
        mongoDbPort: 27017,
    });

    return server;
};

export default app;
