/* eslint-disable sort-keys */
import env from "@fastify/env";


declare module "fastify" {
    export interface FastifyInstance {
        config: {
            PORT: number;
            HOST: string;
            CLP_DB_USER: string;
            CLP_DB_PASS: string;

            RATE_LIMIT_MAX: number;
        };
    }
}


const schema = {
    type: "object",
    required: [
        "PORT",
        "HOST",
        "CLP_DB_USER",
        "CLP_DB_PASS",
    ],
    properties: {
        PORT: {
            type: "number",
            default: 3000,
            minimum: 1,
            maximum: 65535,
        },
        HOST: {
            type: "string",
            minLength: 1,
            default: "localhost",
        },

        // Databases
        CLP_DB_USER: {
            type: "string",
            minLength: 1,
            default: "clp-user",
        },
        CLP_DB_PASS: {
            type: "string",
        },

        // Security
        RATE_LIMIT_MAX: {
            type: "number",
            default: 100,
        },
    },
};

export const autoConfig = {
    confKey: "config",
    schema: schema,

    // Needed to read .env in root folder
    dotenv: {
        path: [
            ".env",
            ".env.local",
        ],
        override: true,
    },

    // Source for the configuration data
    // Optional, default: process.env
    data: process.env,
};

export default env;
