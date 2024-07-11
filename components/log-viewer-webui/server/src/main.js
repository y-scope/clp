import dotenv from "dotenv";
import * as path from "node:path";
import process from "node:process";

import app from "./app.js";


/**
 * Parses environment variables into config values for the application.
 *
 * @return {{CLIENT_DIR: string, CLP_DB_PASS: string, CLP_DB_USER: string,
 * HOST: string, PORT: string}}
 * @throws {Error} if any required environment variable is undefined.
 */
const parseEnvVars = () => {
    dotenv.config({
        path: [
            ".env.local",
            ".env",
        ],
    });

    const {
        CLIENT_DIR, CLP_DB_PASS, CLP_DB_USER, HOST, PORT,
    } = process.env;
    const envVars = {
        CLIENT_DIR, CLP_DB_PASS, CLP_DB_USER, HOST, PORT,
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
    const server = await app({
        clientDir: path.resolve(envVars.CLIENT_DIR),
        fastifyOptions: {
            logger: envToLogger[process.env.NODE_ENV] ?? true,
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

export {parseEnvVars};
