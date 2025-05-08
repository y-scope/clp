import fastifyMongoDb from "@fastify/mongodb";

import settings from "../../../../settings.json" with {type: "json"};

// use MySQLPromisePool if passed promise = true
//declare module 'fastify' {
//    interface FastifyInstance {
//      mongo: MySQLPromisePool
//    }
//}

export const autoConfig = () => {
    return {
        forceClose: true,
        url: `mongodb://${settings.MongoDbHost}:${settings.MongoDbPort}/${settings.MongoDbName}`,
         // Temporary until old webui refactored
        name: "MONGO2"
    }
}

export default fastifyMongoDb
