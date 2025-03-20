import process from "node:process";

import {config as dotenvConfig} from "dotenv";
import type {FastifyServerOptions} from "fastify";


const KNOWN_NODE_ENV = new Set([
    "development",
    "production",
    "test",
]);

type NodeEnv = typeof KNOWN_NODE_ENV extends Set<infer T> ?
    T :
    never;

const NODE_ENV_DEFAULT: NodeEnv = "development";

/**
 * Maps known Node.js environments to Fastify logger configuration options.
 */
const ENV_TO_LOGGER_CONFIG
: Record<NodeEnv, FastifyServerOptions["logger"]> = Object.freeze({
    development: {
        transport: {
            target: "pino-pretty",
        },
    },
    production: true,
    test: false,
});

interface EnvVars {
    NODE_ENV: NodeEnv;

    HOST: string;
    PORT: string;

    CLP_DB_USER: string;
    CLP_DB_PASS: string;
}

/**
 * Validates whether the given value corresponds to a known Node.js environment.
 *
 * @param value
 * @return True if the `value` is a known Node.js environment; otherwise, false.
 */
const isKnownNodeEnv = (value: string | undefined)
: value is NodeEnv => {
    return KNOWN_NODE_ENV.has(value as NodeEnv);
};

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

    Object.entries(mandatoryEnvVars).forEach(([key, value]) => {
        if ("undefined" === typeof value) {
            throw new Error(`Environment variable ${key} must be defined.`);
        }
    });

    return {
        ...mandatoryEnvVars,
        NODE_ENV: isKnownNodeEnv(NODE_ENV) ?
            NODE_ENV :
            NODE_ENV_DEFAULT,
    };
};

export type {NodeEnv};
export {
    ENV_TO_LOGGER_CONFIG,
    parseEnvVars,
};
