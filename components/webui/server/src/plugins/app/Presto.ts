import fp from "fastify-plugin";
import {
    Client,
    ClientOptions,
} from "presto-client";

import settings from "../../../settings.json" with {type: "json"};


// Use process.env.USER if defined, otherwise fallback to "clp-webui"
const CLP_PRESTO_USER_FALLBACK = "clp-webui";

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
        if ("presto" !== settings.ClpQueryEngine) {
            return;
        }

        const clientOptions: ClientOptions = {
            host: settings.PrestoHost,
            port: settings.PrestoPort,
            user: process.env.USER || CLP_PRESTO_USER_FALLBACK,
        };

        fastify.log.info(
            clientOptions,
            "Initializing Presto"
        );
        fastify.decorate("Presto", new Presto(clientOptions));
    },
);
