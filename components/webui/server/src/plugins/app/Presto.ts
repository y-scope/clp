import {CLP_QUERY_ENGINES} from "@webui/common";
import fp from "fastify-plugin";
import {
    Client,
    ClientOptions,
} from "presto-client";

import settings from "../../../settings.json" with {type: "json"};


/**
 * Class to manage Presto client connections.
 */
class Presto {
    readonly client;

    /**
     * @param clientOptions
     */
    constructor (clientOptions: ClientOptions) {
        this.client = new Client(clientOptions);
    }
}

declare module "fastify" {
    interface FastifyInstance {
        Presto?: Presto;
    }
}

export default fp(
    (fastify) => {
        if (CLP_QUERY_ENGINES.PRESTO !== settings.ClpQueryEngine as CLP_QUERY_ENGINES) {
            return;
        }

        const clientOptions: ClientOptions = {
            catalog: fastify.config.PRESTO_CATALOG,
            host: settings.PrestoHost,
            port: settings.PrestoPort,
            schema: fastify.config.PRESTO_SCHEMA,
            user: fastify.config.USER,
        };

        fastify.log.info(
            clientOptions,
            "Initializing Presto client"
        );
        fastify.decorate("Presto", new Presto(clientOptions));
    },
);
