import process from "node:process";

import {config as dotenvConfig} from "dotenv";
import {
    FastifyLoggerOptions,
    PinoLoggerOptions,
} from "fastify/types/logger.js";


type NodeEnv = "development" | "production" | "test";

const KNOWN_NODE_ENVS = new Set<NodeEnv>([
    "development",
    "production",
    "test",
]);

const NODE_ENV_DEFAULT: NodeEnv = "development";

/**
 * Maps known Node.js environments to Fastify logger configuration options.
 */
const ENV_TO_LOGGER
: Record<NodeEnv, boolean | FastifyLoggerOptions & PinoLoggerOptions> = Object.freeze({
    development: {
        transport: {
            target: "pino-pretty",
        },
    },
    production: true,
    test: false,
});

/**
 * Validates whether the given string corresponds to a known Node.js environment.
 *
 * @param value
 * @return True if the `value` is a known Node.js environment; otherwise, false.
 */
const isKnownNodeEnv = (value: string): value is NodeEnv => {
    return KNOWN_NODE_ENVS.has(value as NodeEnv);
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
 * @return
 * @throws {Error} if any required environment variable is undefined.
 */
const parseEnvVars = (): EnvVars => {
    dotenvConfig({
        path: [
            ".env.local",
            ".env",
        ],
    });

    const {
        NODE_ENV, CLP_DB_USER, CLP_DB_PASS, HOST, PORT,
    } = process.env;
    const mandatoryEnvVars = {
        CLP_DB_USER, CLP_DB_PASS, HOST, PORT,
    } as EnvVars;

    // Check for mandatory environment variables
    for (const [key, value] of Object.entries(mandatoryEnvVars)) {
        if ("undefined" === typeof value) {
            throw new Error(`Environment variable ${key} must be defined.`);
        }
    }

    // Sanitize other environment variables
    const sanitizedEnvVars = {
        NODE_ENV: NODE_ENV_DEFAULT as NodeEnv,
    };

    if ("undefined" === typeof NODE_ENV || false === isKnownNodeEnv(NODE_ENV)) {
        console.log("NODE_ENV is not set, or the configured value is not known." +
            `Using default: ${NODE_ENV_DEFAULT}`);
    } else {
        sanitizedEnvVars.NODE_ENV = NODE_ENV;
    }

    return {
        ...mandatoryEnvVars,
        ...sanitizedEnvVars,
    };
};

export type {NodeEnv};
export {
    ENV_TO_LOGGER,
    parseEnvVars,
};
