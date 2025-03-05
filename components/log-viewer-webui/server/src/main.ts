import process from "node:process";

import app from "./app.js";
import {
    ENV_TO_LOGGER,
    parseEnvVars,
} from "./utils/env.js";


/**
 * Sets up and runs the server.
 */
const main = async () => {
    const envVars = parseEnvVars();
    const server = await app({
        fastifyOptions: {
            logger: ENV_TO_LOGGER[envVars.NODE_ENV],
        },
        sqlDbPass: envVars.CLP_DB_PASS,
        sqlDbUser: envVars.CLP_DB_USER,
    });

    try {
        await server.listen({host: envVars.HOST, port: Number(envVars.PORT)});
    } catch (e) {
        server.log.error(e);
        process.exit(1);
    }
};

main().catch((e: unknown) => {
    console.error(e);
    process.exit(1);
});
