import path from "node:path";
import {fileURLToPath} from "node:url";

import env from "@fastify/env";


declare module "fastify" {
    export interface FastifyInstance {
        config: {
            PORT: number;
            HOST: string;
            USER: string;
            CLP_DB_USER: string;
            CLP_DB_PASS: string;
            CLP_STREAM_OUTPUT_AWS_ACCESS_KEY_ID: string;
            CLP_STREAM_OUTPUT_AWS_SECRET_ACCESS_KEY: string;
            PRESTO_CATALOG: string;
            PRESTO_SCHEMA: string;
            RATE_LIMIT: number;
        };
    }
}


const compiledDirname = path.dirname(fileURLToPath(import.meta.url));
const sourceWebuiRoot = path.resolve(compiledDirname, "../../../../..");

const schema = {
    type: "object",
    required: [
        "PORT",
        "HOST",
        "CLP_DB_USER",
        "CLP_DB_PASS",
    ],
    /* eslint-disable sort-keys */
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

        // S3
        CLP_STREAM_OUTPUT_AWS_ACCESS_KEY_ID: {
            type: "string",
            default: "",
        },
        CLP_STREAM_OUTPUT_AWS_SECRET_ACCESS_KEY: {
            type: "string",
            default: "",
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
        /* eslint-enable sort-keys */
    },
};

export const autoConfig = {
    confKey: "config",
    schema: schema,

    // Load secrets and env-only overrides from the WebUI root.
    dotenv: {
        path: [
            path.join(sourceWebuiRoot, ".env"),
            path.join(sourceWebuiRoot, ".env.local"),
        ],
        override: true,
    },

    // Source for the configuration data
    // Optional, default: process.env
    data: process.env,
};

export default env;
