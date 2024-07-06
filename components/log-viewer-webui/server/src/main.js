import dotenv from "dotenv";
import * as path from "node:path";
import process from "node:process";

import app from "./app.js";


/**
 * Parses environment variables into config values for the application.
 *
 * @return {{HOST: string, PORT: string, CLIENT_DIR: string}}
 * @throws {Error} if any required environment variable is undefined.
 */
const parseEnvVars = () => {
    dotenv.config({
        path: ".env",
    });

    const {
        HOST, PORT, CLIENT_DIR,
    } = process.env;
    const envVars = {
        HOST, PORT, CLIENT_DIR,
    };

    // Check for mandatory environment variables
    for (const [key, value] of Object.entries(envVars)) {
        if ("undefined" === typeof value) {
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

    const envVars = parseEnvVars();
    const server = await app(path.resolve(envVars.CLIENT_DIR), {
        logger: envToLogger[process.env.NODE_ENV] ?? true,
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
