import dotenv from "dotenv";
import process from "node:process";

import app from "./app.js";


/**
 * Parses environment variables into config values for the application.
 *
 * @return {{PORT: string, HOST: string}}
 * @throws {Error} if any required environment variables is undefined.
 */
const parseEnvVars = () => {
    dotenv.config({
        path: ".env",
    });

    const {
        HOST, PORT,
    } = process.env;
    const envVars = {
        HOST, PORT,
    };

    // Check for mandatory environment variables
    for (const [key, value] of Object.entries(envVars)) {
        // eslint-disable-next-line no-undefined
        if (undefined === value) {
            throw new Error(`Environment variable ${key} must be defined.`);
        }
    }

    return envVars;
};

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
    const server = await app({
        logger: envToLogger[process.env.NODE_ENV] ?? true,
    });

    try {
        const envVars = parseEnvVars();
        await server.listen({host: envVars.HOST, port: parseInt(envVars.PORT, 10)});
    } catch (e) {
        server.log.error(e);
        process.exit(1);
    }
};

main().catch((e) => {
    console.error(e);
    process.exit(1);
});
