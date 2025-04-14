import process from "node:process";

import {
    Static,
    Type,
} from "@sinclair/typebox";
import {
    AssertError,
    Value,
} from "@sinclair/typebox/value";
import {config as dotenvConfig} from "dotenv";
import type {FastifyServerOptions} from "fastify";


const NodeEnv = Type.Union([
    Type.Literal("development"),
    Type.Literal("production"),
    Type.Literal("test"),
]);
const NODE_ENV_DEFAULT = "development";

/**
 * Maps known Node.js environments to Fastify logger configuration options.
 */
const ENV_TO_LOGGER_MAP: Record<
    Static<typeof NodeEnv>,
    FastifyServerOptions["logger"]
> = Object.freeze({
    development: {
        transport: {
            target: "pino-pretty",
        },
    },
    production: true,
    test: false,
});
const EnvVars = Type.Required(
    Type.Object({
        CLP_DB_PASS: Type.String(),
        CLP_DB_USER: Type.String({minLength: 1}),
        HOST: Type.String({minLength: 1}),
        NODE_ENV: NodeEnv,
        PORT: Type.Number({minimum: 1, maximum: 65535}),
    })
);

/**
 * Parses environment variables into config values for the application.
 *
 * @return
 * @throws {Error} If any required environment variable is undefined or invalid.
 */
const parseEnvVars = (): Static<typeof EnvVars> => {
    dotenvConfig({
        path: [
            ".env.local",
            ".env",
        ],
    });
    process.env.NODE_ENV ??= NODE_ENV_DEFAULT;
    try {
        return Value.Parse(EnvVars, process.env);
    } catch (e: unknown) {
        let message = "Environment variable is missing:";
        if (e instanceof AssertError) {
            const errors = [...Value.Errors(EnvVars, process.env)]
                .map((err) => `${err.path}: ${err.message}`)
                .join("; ");

            message += ` Details: ${errors}`;
        }
        throw new Error(message);
    }
};


export type {EnvVars};
export {
    ENV_TO_LOGGER_MAP,
    parseEnvVars,
};
