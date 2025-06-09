import {
    FastifyInstance,
    FastifyPluginAsync,
} from "fastify";

import settings from "../settings.json" with {type: "json"};
import DbManager from "./plugins/DbManager.js";
import MongoSocketIoServer from "./plugins/MongoSocketIoServer/index.js";
import S3Manager from "./plugins/S3Manager.js";
import exampleRoutes from "./routes/example.js";
import queryRoutes from "./routes/query.js";
import staticRoutes from "./routes/static.js";


interface AppPluginOptions {
    sqlDbUser: string;
    sqlDbPass: string;
}

/**
 * Creates the Fastify app with the given options.
 *
 * TODO: Once old webui code is refactored to new modlular fastify style, this plugin should be
 * removed.
 *
 * @param fastify
 * @param opts
 * @return
 */
const FastifyV1App: FastifyPluginAsync<AppPluginOptions> = async (
    fastify: FastifyInstance,
    opts: AppPluginOptions
) => {
    const {sqlDbUser, sqlDbPass} = opts;
    if ("test" !== process.env.NODE_ENV) {
        await fastify.register(DbManager, {
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
                streamFilesCollectionName: settings.MongoDbStreamFilesCollectionName,
                port: settings.MongoDbPort,
            },
        });
        await fastify.register(
            S3Manager,
            {
                region: settings.StreamFilesS3Region,
                profile: settings.StreamFilesS3Profile,
            }
        );
        await fastify.register(MongoSocketIoServer, {
            host: settings.MongoDbHost,
            port: settings.MongoDbPort,
            database: settings.MongoDbName,
        });
    }

    // Register the routes
    await fastify.register(staticRoutes);
    await fastify.register(exampleRoutes);
    await fastify.register(queryRoutes);
};

export default FastifyV1App;
