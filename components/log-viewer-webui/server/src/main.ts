import process from "node:process";

import app from "./app.js";
import {
    ENV_TO_LOGGER_MAP,
    parseEnvVars,
} from "./utils/env.js";


/**
 * Sets up and runs the server.
 */
const main = async () => {
    const envVars = parseEnvVars();
    const loggerConfig = ENV_TO_LOGGER_MAP[envVars.NODE_ENV];
    const server = await app({
        fastifyOptions: {
            ...(loggerConfig && {logger: loggerConfig}),
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
