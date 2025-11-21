import {brotliCompressSync} from "node:zlib";

import type {MySQLPromisePool} from "@fastify/mysql";
import {encode} from "@msgpack/msgpack";
import {FastifyInstance} from "fastify";
import fp from "fastify-plugin";
import {ResultSetHeader} from "mysql2";

import settings from "../../../../settings.json" with {type: "json"};
import {COMPRESSION_JOBS_TABLE_COLUMN_NAMES} from "../../../typings/compression.js";
import {CompressionJobConfig} from "./typings.js";


/**
 * Class for managing compression jobs in the CLP package compression scheduler database.
 */
class CompressionJobDbManager {
    readonly #sqlPool: MySQLPromisePool;

    readonly #tableName: string;

    private constructor (sqlPool: MySQLPromisePool, tableName: string) {
        this.#sqlPool = sqlPool;
        this.#tableName = tableName;
    }

    /**
     * Creates a new CompressionJobDbManager.
     *
     * @param fastify
     * @return
     */
    static create (fastify: FastifyInstance): CompressionJobDbManager {
        return new CompressionJobDbManager(fastify.mysql, settings.SqlDbCompressionJobsTableName);
    }

    /**
     * Submits a compression job to the database.
     *
     * @param jobConfig
     * @return The job's ID.
     * @throws {Error} on error.
     */
    async submitJob (jobConfig: CompressionJobConfig): Promise<number> {
        const [result] = await this.#sqlPool.query<ResultSetHeader>(
            `
            INSERT INTO ${this.#tableName} (
               ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.JOB_CONFIG}
            )
            VALUES (?)
            `,
            [
                Buffer.from(brotliCompressSync(encode(jobConfig))),
            ]
        );

        return result.insertId;
    }
}

declare module "fastify" {
    interface FastifyInstance {
        CompressionJobDbManager: CompressionJobDbManager;
    }
}

export default fp(
    (fastify) => {
        fastify.decorate("CompressionJobDbManager", CompressionJobDbManager.create(fastify));
    },
    {
        name: "CompressionJobDbManager",
    }
);
