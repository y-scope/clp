import fastify from "fastify";
import * as path from "node:path";
import process from "node:process";

import {fastifyStatic} from "@fastify/static";

import DbManager from "./DbManager.js";
import exampleRoutes from "./routes/example.js";
import queryRoutes from "./routes/query.js";


/**
 * Creates the Fastify #fastify with the given options.
 *
 * @param {object} props
 * @param {string} props.clientDir Absolute path to the client directory to serve when in
 * running in a production environment.
 * @param {string} props.irDataDir
 * @param {string} props.logViewerDir
 * @param {import("fastify").FastifyServerOptions} props.fastifyOptions
 * @param {string} props.dbPass The MySQL database password.
 * @param {string} props.dbUser The MySQL database user.
 * @return {Promise<import("fastify").FastifyInstance>}
 */
const app = async ({
    clientDir,
    irDataDir,
    logViewerDir,
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

    await server.register(fastifyStatic, {
        prefix: "/ir",
        root: irDataDir,
    });
    await server.register(fastifyStatic, {
        prefix: "/log-viewer",
        root: logViewerDir,
        decorateReply: false,
    });

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
            irFilesCollectionName: "ir-files",
            port: 27017,
        },
    });

    await server.register(exampleRoutes);
    await server.register(queryRoutes);

    return server;
};

export default app;
