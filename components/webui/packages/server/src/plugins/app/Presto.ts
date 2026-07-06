import {CLP_QUERY_ENGINES} from "@webui/common/config";
import fp from "fastify-plugin";
import {
    Client,
    ClientOptions,
} from "presto-client";

import {
    publicSettings,
    serverSettings,
} from "../../settings.js";


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
        if (CLP_QUERY_ENGINES.PRESTO !== publicSettings.ClpQueryEngine) {
            return;
        }

        const {PrestoHost, PrestoPort} = serverSettings;
        if (null === PrestoHost || null === PrestoPort) {
            fastify.log.warn(
                "Presto query engine is configured but PrestoHost/PrestoPort are not set; " +
                "skipping Presto client initialization."
            );

            return;
        }

        const clientOptions: ClientOptions = {
            catalog: fastify.config.PRESTO_CATALOG,
            host: PrestoHost,
            port: PrestoPort,
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
