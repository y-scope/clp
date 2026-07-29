import {
    fastifyMysql,
    MySQLPromisePool,
} from "@fastify/mysql";
import {FastifyInstance} from "fastify";

import {serverSettings} from "../../settings.js";


// The typing of `@fastify/mysql` needs to be manually specified.
// See https://github.com/fastify/fastify-mysql#typescript
declare module "fastify" {
    interface FastifyInstance {
        mysql: MySQLPromisePool;
    }
}

export const autoConfig = (fastify: FastifyInstance) => {
    return {
        database: serverSettings.SqlDbName,
        host: serverSettings.SqlDbHost,
        password: fastify.config.CLP_DB_PASS,
        port: serverSettings.SqlDbPort,
        promise: true,
        user: fastify.config.CLP_DB_USER,
    };
};

export default fastifyMysql;
