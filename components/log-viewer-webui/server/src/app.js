import fastify from "fastify";
import * as path from "node:path";
import process from "node:process";
import {fileURLToPath} from "node:url";

import {fastifyStatic} from "@fastify/static";

import settings from "../settings.json" with {type: "json"};
import DbManager from "./DbManager.js";
import exampleRoutes from "./routes/example.js";
import queryRoutes from "./routes/query.js";


/**
 * Creates the Fastify app with the given options.
 *
 * @param {object} props
 * @param {import("fastify").FastifyServerOptions} props.fastifyOptions
 * @param {string} props.sqlDbPass
 * @param {string} props.sqlDbUser
 * @return {Promise<import("fastify").FastifyInstance>}
 */
const app = async ({
    fastifyOptions,
    sqlDbPass,
    sqlDbUser,
}) => {
    const server = fastify(fastifyOptions);
    const filename = fileURLToPath(import.meta.url);
    const dirname = path.dirname(filename);
    const parentDirname = path.resolve(dirname, "..");

    if ("test" !== process.env.NODE_ENV) {
        let irFilesDir = settings.IrFilesDir;
        if (false === path.isAbsolute(irFilesDir)) {
            irFilesDir = path.resolve(parentDirname, irFilesDir);
        }
        await server.register(fastifyStatic, {
            prefix: "/ir",
            root: irFilesDir,
        });

        await server.register(DbManager, {
            mysqlConfig: {
                database: settings.SqlDbName,
                host: settings.SqlDbHost,
                password: sqlDbPass,
                port: settings.SqlDbPort,
                queryJobsTableName: settings.SqlDbQueryJobsTableName,
                user: sqlDbUser,
            },
            mongoConfig: {
                database: settings.MongoDbName,
                host: settings.MongoDbHost,
                irFilesCollectionName: settings.MongoDbIrFilesCollectionName,
                port: settings.MongoDbPort,
            },
        });
    }

    if ("production" === process.env.NODE_ENV) {
        // In the development environment, we expect the client to use a separate webserver that
        // supports live reloading.
        if (false === path.isAbsolute(settings.ClientDir)) {
            throw new Error("`clientDir` must be an absolute path.");
        }

        await server.register(fastifyStatic, {
            prefix: "/",
            root: settings.ClientDir,
            decorateReply: false,
        });
    }

    await server.register(exampleRoutes);
    await server.register(queryRoutes);

    return server;
};

export default app;
