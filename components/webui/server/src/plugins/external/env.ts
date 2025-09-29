/* eslint-disable sort-keys */
import env from "@fastify/env";


declare module "fastify" {
    export interface FastifyInstance {
        config: {
            PORT: number;
            HOST: string;
            USER: string;
            CLP_DB_USER: string;
            CLP_DB_PASS: string;
            PRESTO_CATALOG: string;
            PRESTO_SCHEMA: string;
            RATE_LIMIT: number;
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
        // Network
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

        // System
        USER: {
            type: "string",
            default: "clp-webui",
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

        // Presto
        PRESTO_CATALOG: {
            type: "string",
            default: "clp",
        },
        PRESTO_SCHEMA: {
            type: "string",
            default: "default",
        },

        // Security
        RATE_LIMIT: {
            type: "number",
            default: 1_000,
            minimum: 1,
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
