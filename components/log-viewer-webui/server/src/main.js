import process from "node:process";

import app from "./app.js";
import {parseEnvVars} from "./utils.js";


/**
 * Sets up and runs the server.
 */
const main = async () => {
    const envToLogger = {
        development: {
            transport: {
                target: "pino-pretty",
            },
        },
        production: true,
        test: false,
    };

    const envVars = parseEnvVars();
    const server = await app({
        fastifyOptions: {
            logger: envToLogger[process.env.NODE_ENV] ?? true
        },
        dbUser: envVars.CLP_DB_USER,
        dbPass: envVars.CLP_DB_PASS,
    });

    try {
        await server.listen({host: envVars.HOST, port: Number(envVars.PORT)});
    } catch (e) {
        server.log.error(e);
        process.exit(1);
    }
};

main().catch((e) => {
    console.error(e);
    process.exit(1);
});
