import fastifyMongoDb from "@fastify/mongodb";

import settings from "../../../../settings.json" with {type: "json"};


export const autoConfig = () => {
    return {
        forceClose: true,
        url: `mongodb://${settings.MongoDbHost}:${settings.MongoDbPort}/${settings.MongoDbName}`,
    };
};

export default fastifyMongoDb;
