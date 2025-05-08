/*import process from "node:process";

import {
    fastify,
    FastifyInstance,
    FastifyServerOptions,
} from "fastify";

import settings from "../settings.json" with {type: "json"};
import DbManager from "./plugins/DbManager.js";
import S3Manager from "./plugins/S3Manager.js";
import exampleRoutes from "./routes/example.js";
import queryRoutes from "./routes/query.js";
import staticRoutes from "./routes/static.js";

import fastifyMongo from '@fastify/mongodb';

import {
    fastifyMysql,
    isMySQLPromisePool,
    MySQLPromisePool,
} from "@fastify/mysql";


interface AppProps {
    fastifyOptions: FastifyServerOptions;
    sqlDbUser: string;
    sqlDbPass: string;
}

/**
 * Creates the Fastify app with the given options.
 *
 * @param props
 * @param props.fastifyOptions
 * @param props.sqlDbUser
 * @param props.sqlDbPass
 * @return The created Fastify instance.
 */
/*
const app = async ({
    fastifyOptions,
    sqlDbUser,
    sqlDbPass,
}: AppProps): Promise<FastifyInstance> => {
    const server = fastify(fastifyOptions);

    if ("test" !== process.env.NODE_ENV) {
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
        await server.register(
            S3Manager,
            {
                region: settings.StreamFilesS3Region,
                profile: settings.StreamFilesS3Profile,
            }
        );
        await server.register(fastifyMongo, {
            forceClose: true,
            url: `mongodb://${settings.MongoDbHost}:${settings.MongoDbPort}/${settings.MongoDbName}`,
            name: 'MONGO2'
        })

        server
        .register(fastifyMysql, {
          promise: true,
          connectionString: `mysql://${sqlDbUser}:${sqlDbPass}@${settings.SqlDbHost}:` +
            `${settings.SqlDbPort}/${settings.SqlDbName}`,
            name: 'SQL2'
        })
        .after(async function () {
          if (isMySQLPromisePool(server.mysql)) {
            const mysql = server.mysql
            const con = await mysql.getConnection();
            con.release();
            mysql.pool.end();
          }
        });

    }

    await server.register(staticRoutes);
    await server.register(exampleRoutes);
    await server.register(queryRoutes);

    return server;
};

// if you passed promise = true
declare module 'fastify' {
    interface FastifyInstance {
      mysql: MySQLPromisePool
    }
  }

export default app;
*/