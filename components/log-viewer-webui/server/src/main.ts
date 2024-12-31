import dotenv from "dotenv";
import process from "node:process";

import app from "./app.js";


type NodeEnv = "development" | "production" | "test";

const NODE_ENV_DEFAULT: NodeEnv = "development";

const ENV_TO_LOGGER: Record<NodeEnv, any> = Object.freeze({
    development: {
        transport: {
            target: "pino-pretty",
        },
    },
    production: true,
    test: false,
});

const isKnownNodeEnv = (value: string): value is NodeEnv => {
    return ["development", "production", "test"].includes(value);
};

interface EnvVars {
    NODE_ENV: NodeEnv;

    HOST: string;
    PORT: string;

    CLP_DB_USER: string;
    CLP_DB_PASS: string;
}

/**
 * Parses environment variables into config values for the application.
 *
 * @return {{CLP_DB_USER: string, CLP_DB_PASS: string, HOST: string, PORT: string}}
 * @throws {Error} if any required environment variable is undefined.
 */
const parseEnvVars = (): EnvVars => {
    dotenv.config({
        path: [
            ".env.local",
            ".env",
        ],
    });

    /* eslint-disable sort-keys */
    const {
        NODE_ENV, CLP_DB_USER, CLP_DB_PASS, HOST, PORT,
    } = process.env;
    const mandatoryEnvVars = {
        CLP_DB_USER, CLP_DB_PASS, HOST, PORT,
    } as EnvVars;
    /* eslint-enable sort-keys */

    // Check for mandatory environment variables
    for (const [key, value] of Object.entries(mandatoryEnvVars)) {
        if ("undefined" === typeof value) {
            throw new Error(`Environment variable ${key} must be defined.`);
        }
    }

    // Sanitize other environment variables
    let sanitizedEnvVars = {
        NODE_ENV: NODE_ENV_DEFAULT as NodeEnv
    }
    if (typeof NODE_ENV === "undefined" || false === isKnownNodeEnv(NODE_ENV)) {
        console.log(`NODE_ENV is not set, or the configured value is not known.`+
            `Using default: ${NODE_ENV_DEFAULT}`);
    } else {
        sanitizedEnvVars.NODE_ENV = NODE_ENV;
    }

    return {
        ...mandatoryEnvVars,
        ...sanitizedEnvVars
    };
};

/**
 * Sets up and runs the server.
 */
const main = async () => {
    const envVars = parseEnvVars();
    const server = await app({
        fastifyOptions: {
            logger: ENV_TO_LOGGER[envVars.NODE_ENV] ?? true,
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
