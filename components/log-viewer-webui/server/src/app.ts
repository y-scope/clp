import process from "node:process";

import {
    FastifyPluginAsync
} from "fastify";

import settings from "../settings.json" with {type: "json"};
import DbManager from "./plugins/DbManager.js";
import S3Manager from "./plugins/S3Manager.js";
import exampleRoutes from "./routes/example.js";
import queryRoutes from "./routes/query.js";
import staticRoutes from "./routes/static.js";

interface AppPluginOptions {
    sqlDbUser: string;
    sqlDbPass: string;
}

const AppPlugin: FastifyPluginAsync<AppPluginOptions> = async (server, opts) => {
    const { sqlDbUser, sqlDbPass } = opts;

    // Ensure environment is not "test"
    if (process.env.NODE_ENV !== "test") {
        // Register DbManager and S3Manager plugins
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
                streamFilesCollectionName: settings.MongoDbStreamFilesCollectionName,
                port: settings.MongoDbPort,
            },
        });

        await server.register(S3Manager, {
            region: settings.StreamFilesS3Region,
            profile: settings.StreamFilesS3Profile,
        });
    }

    // Register the routes
    await server.register(staticRoutes);
    await server.register(exampleRoutes);
    await server.register(queryRoutes);
};

export default AppPlugin;