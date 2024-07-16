import dotenv from "dotenv";
import * as path from "node:path";
import process from "node:process";

import app from "./app.js";


/**
 * Parses environment variables into config values for the application.
 *
 * @return {{CLIENT_DIR: string, IR_DATA_DIR: string, LOG_VIEWER_DIR: string,
 * CLP_DB_PASS: string, CLP_DB_USER: string, HOST: string, PORT: string}}
 * @throws {Error} if any required environment variable is undefined.
 */
const parseEnvVars = () => {
    dotenv.config({
        path: [
            ".env.local",
            ".env",
        ],
    });

    /* eslint-disable sort-keys */
    const {
        CLIENT_DIR, IR_DATA_DIR, LOG_VIEWER_DIR, CLP_DB_PASS, CLP_DB_USER, HOST, PORT,
    } = process.env;
    const envVars = {
        CLIENT_DIR, IR_DATA_DIR, LOG_VIEWER_DIR, CLP_DB_PASS, CLP_DB_USER, HOST, PORT,
    };
    /* eslint-enable sort-keys */

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
        fastifyOptions: {
            logger: envToLogger[process.env.NODE_ENV] ?? true,
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

main().catch((e) => {
    console.error(e);
    process.exit(1);
});

export {parseEnvVars};
