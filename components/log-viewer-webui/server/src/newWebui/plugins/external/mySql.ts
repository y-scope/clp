import {
    fastifyMysql,
    MySQLPromisePool,
} from "@fastify/mysql";
import { FastifyInstance } from 'fastify'

import settings from "../../../../settings.json" with {type: "json"};
// use MySQLPromisePool if passed promise = true
declare module 'fastify' {
    interface FastifyInstance {
      mysql: MySQLPromisePool;
      MYSQL2: MySQLPromisePool;
    }
}


export const autoConfig = (fastify: FastifyInstance) => {
    return {
        promise: true,
        host: settings.SqlDbHost,
        user: fastify.config.CLP_DB_USER,
        password: fastify.config.CLP_DB_PASS,
        database: settings.SqlDbName,
        port: Number(settings.SqlDbPort),
        // Temporary until old webui refactored
        name: "MYSQL2"
    }
}

export default fastifyMysql

