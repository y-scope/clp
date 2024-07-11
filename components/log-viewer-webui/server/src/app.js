import fastify from "fastify";
import * as path from "node:path";
import process from "node:process";

import {fastifyStatic} from "@fastify/static";

import DbManager from "./DbManager.js";
import exampleRoutes from "./routes/examples.js";


/**
 * Creates the Fastify app with the given options.
 *
 * @param {object} props
 * @param {string} props.clientDir Absolute path to the client directory to serve when in
 * running in a production environment.
 * @param {import("fastify").FastifyServerOptions} props.fastifyOptions
 * @param {string} props.dbPass The MySQL database password.
 * @param {string} props.dbUser The MySQL database user.
 * @return {Promise<import("fastify").FastifyInstance>}
 */
const app = async ({
    clientDir,
    fastifyOptions,
    dbPass,
    dbUser,
}) => {
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
    await server.register(DbManager, {
        mysqlConfig: {
            database: "clp-db",
            host: "127.0.0.1",
            password: dbPass,
            port: 3306,
            queryJobsTableName: "query_jobs",
            user: dbUser,
        },
        mongoConfig: {
            database: "clp-query-results",
            host: "127.0.0.1",
            port: 27017,
            statsCollectionName: "stats",
        },
    });

    return server;
};

export default app;
