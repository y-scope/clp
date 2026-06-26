import fastifyMongoDb from "@fastify/mongodb";

import {serverSettings} from "../../settings.js";


export const autoConfig = () => {
    return {
        forceClose: true,
        url: `mongodb://${serverSettings.MongoDbHost}:${serverSettings.MongoDbPort}/${
            serverSettings.MongoDbName}?directConnection=true`,
    };
};

export default fastifyMongoDb;
