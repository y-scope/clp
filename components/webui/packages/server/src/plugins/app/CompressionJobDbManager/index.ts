import {
    constants as zlibConstants,
    zstdCompressSync,
    type ZstdOptions,
} from "node:zlib";

import type {MySQLPromisePool} from "@fastify/mysql";
import {encode} from "@msgpack/msgpack";
import type {ClpIoConfig} from "@webui/common/schemas/compression";
import type {FastifyInstance} from "fastify";
import fp from "fastify-plugin";
import type {ResultSetHeader} from "mysql2";

import {publicSettings} from "../../../settings.js";
import {COMPRESSION_JOBS_TABLE_COLUMN_NAMES} from "../../../typings/compression.js";


const ZSTD_COMPRESSION_LEVEL = 3;

const ZSTD_COMPRESSION_OPTIONS: ZstdOptions = {
    params: {
        [zlibConstants.ZSTD_c_compressionLevel]: ZSTD_COMPRESSION_LEVEL,
    },
};


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
        return new CompressionJobDbManager(
            fastify.mysql,
            publicSettings.SqlDbCompressionJobsTableName
        );
    }

    /**
     * Submits a compression job to the database.
     *
     * @param jobConfig
     * @return The job's ID.
     * @throws {Error} on error.
     */
    async submitJob (jobConfig: ClpIoConfig): Promise<number> {
        const [result] = await this.#sqlPool.query<ResultSetHeader>(
            `
            INSERT INTO ${this.#tableName} (
               ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.JOB_CONFIG}
            )
            VALUES (?)
            `,
            [
                Buffer.from(zstdCompressSync(encode(jobConfig), ZSTD_COMPRESSION_OPTIONS)),
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
