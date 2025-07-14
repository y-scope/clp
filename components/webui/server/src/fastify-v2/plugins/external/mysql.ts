import {
    fastifyMysql,
    MySQLPromisePool,
} from "@fastify/mysql";
import {FastifyInstance} from "fastify";

import settings from "../../../../settings.json" with {type: "json"};


// The typing of `@fastify/mysql` needs to be manually specified.
// See https://github.com/fastify/fastify-mysql#typescript
declare module "fastify" {
    interface FastifyInstance {
        mysql: MySQLPromisePool;
    }
}

export const autoConfig = (fastify: FastifyInstance) => {
    return {
        database: settings.SqlDbName,
        host: settings.SqlDbHost,
        password: fastify.config.CLP_DB_PASS,
        port: Number(settings.SqlDbPort),
        promise: true,
        user: fastify.config.CLP_DB_USER,
    };
};

export default fastifyMysql;
