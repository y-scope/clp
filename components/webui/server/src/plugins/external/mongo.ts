import fastifyMongoDb from "@fastify/mongodb";

import settings from "../../../settings.json" with {type: "json"};


export const autoConfig = () => {
    let url = `mongodb://${settings.MongoDbHost}:${settings.MongoDbPort}/${settings.MongoDbName}?directConnection=true`;
    const mongoDbTls = settings.MongoDbTls as boolean;
    const mongoDbTlsCaFile = settings.MongoDbTlsCaFile as string | null;
    if (mongoDbTls) {
        url += "&tls=true";
        if (mongoDbTlsCaFile) {
            url += `&tlsCAFile=${mongoDbTlsCaFile}`;
        }
    }
    return {
        forceClose: true,
        url,
    };
};

export default fastifyMongoDb;
