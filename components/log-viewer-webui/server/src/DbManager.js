import fastifyPlugin from "fastify-plugin";

import fastifyMongo from "@fastify/mongodb";
import fastifyMysql from "@fastify/mysql";
import msgpack from "@msgpack/msgpack";

import {sleep} from "./utils.js";


/**
 * Interval in milliseconds for polling the completion status of a job.
 */
const JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS = 0.5;

/**
 * Enum of the `query_jobs` table's column names.
 *
 * @enum {string}
 */
const QUERY_JOBS_TABLE_COLUMN_NAMES = Object.freeze({
    ID: "id",
    STATUS: "status",
    TYPE: "type",
    JOB_CONFIG: "job_config",
});

/* eslint-disable sort-keys */
let enumQueryJobStatus;
/**
 * Enum of job statuses, matching the `QueryJobStatus` class in
 * `job_orchestration.query_scheduler.constants`.
 *
 * @enum {number}
 */
const QUERY_JOB_STATUS = Object.freeze({
    PENDING: (enumQueryJobStatus = 0),
    RUNNING: ++enumQueryJobStatus,
    SUCCEEDED: ++enumQueryJobStatus,
    FAILED: ++enumQueryJobStatus,
    CANCELLING: ++enumQueryJobStatus,
    CANCELLED: ++enumQueryJobStatus,
});
/* eslint-enable sort-keys */

/**
 * List of states that indicates the job has running actions.
 */
const QUERY_JOB_STATUS_WAITING_STATES = Object.freeze([
    QUERY_JOB_STATUS.PENDING,
    QUERY_JOB_STATUS.RUNNING,
    QUERY_JOB_STATUS.CANCELLING,
]);

/* eslint-disable sort-keys */
let enumQueryType;
/**
 * Enum of job type, matching the `QueryJobType` class in
 * `job_orchestration.query_scheduler.constants`.
 *
 * @enum {number}
 */
const QUERY_JOB_TYPE = Object.freeze({
    SEARCH_OR_AGGREGATION: (enumQueryType = 0),
    EXTRACT_IR: ++enumQueryType,
});
/* eslint-enable sort-keys */

/**
 * Class representing the database manager.
 */
class DbManager {
    /**
     * @type {import("fastify").FastifyInstance | {mysql: import("@fastify/mysql").MySQLPromisePool}}
     */
    #fastify;

    /**
     * @type {import("@fastify/mysql").MySQLPromisePool}
     */
    #mysqlConnectionPool;

    #queryJobsTableName;

    /**
     * Creates a DbManager.
     *
     * @param {import("fastify").FastifyInstance} app The Fastify application instance
     * @param {object} dbConfig The database configuration
     * @param {object} dbConfig.mysqlConfig The MySQL configuration
     * @param {object} dbConfig.mongoConfig The MongoDB configuration
     */
    constructor (app, dbConfig) {
        this.#fastify = app;
        this.#initMySql(dbConfig.mysqlConfig);
        this.#initMongo(dbConfig.mongoConfig);
    }

    async awaitJobCompletion (jobId) {
        while (true) {
            let rows;
            try {
                const [queryRows] = await this.#mysqlConnectionPool.query(
                    `
                    SELECT ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS}
                    FROM ${this.#queryJobsTableName}
                    WHERE ${QUERY_JOBS_TABLE_COLUMN_NAMES.ID} = ?
                    `,
                    jobId,
                );

                rows = queryRows;
            } catch (e) {
                throw new Error(`Failed to query status for job ${jobId} - ${e}`);
            }
            if (0 === rows.length) {
                throw new Error(`Job ${jobId} not found in database.`);
            }
            const status = rows[0][QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS];

            if (false === QUERY_JOB_STATUS_WAITING_STATES.includes(status)) {
                if (QUERY_JOB_STATUS.CANCELLED === status) {
                    throw new Error(`Job ${jobId} was cancelled.`);
                } else if (QUERY_JOB_STATUS.SUCCEEDED !== status) {
                    throw new Error(`Job ${jobId} exited with unexpected status=${status}: ` +
                        `${Object.keys(QUERY_JOB_STATUS)[status]}.`);
                }
                break;
            }

            await sleep(JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS);
        }
    }

    /**
     * Inserts an Extract IR job into MySQL.
     *
     * @param {object} config The job configuration.
     * @return {Promise<number|null>} The job id of the inserted query or null if an error occurred.
     */
    async insertExtractIrJob (config) {
        let jobId;
        try {
            const [result] = await this.#mysqlConnectionPool.query(
                `INSERT INTO ${this.#queryJobsTableName} (job_config, type)
             VALUES (?, ?)`,
                [
                    Buffer.from(msgpack.encode(config)),
                    QUERY_JOB_TYPE.EXTRACT_IR,
                ]
            );

            ({insertId: jobId} = result);
            await this.awaitJobCompletion(jobId);
        } catch (e) {
            this.#fastify.log.error(e);

            return null;
        }

        return jobId;
    }

    /**
     * Retrieves the extracted IR split metadata for a given original file ID. When there are
     * multiple splits for a single original file, only the metadata of the split containing a given
     * message index will be returned.
     *
     * @param {string} origFileId
     * @param {number} msgIdx
     * @return {Promise<object>} A promise that resolves to the extracted IR metadata.
     */
    async getExtractIrMetadata (origFileId, msgIdx) {
        return await this.irFilesCollection.findOne({
            orig_file_id: origFileId,
            begin_msg_ix: {$lte: msgIdx},
            end_msg_ix: {$gt: msgIdx},
        });
    }

    /**
     * Initializes MySQL connection.
     *
     * @param {object} config
     * @param {string} config.user
     * @param {string} config.password
     * @param {string} config.host
     * @param {number} config.port
     * @param {string} config.database
     * @param {string} config.queryJobsTableName
     */
    #initMySql (config) {
        this.#fastify.register(fastifyMysql, {
            promise: true,
            connectionString: `mysql://${config.user}:${config.password}@${config.host}:` +
                `${config.port}/${config.database}`,
        }).after(async (err) => {
            if (err) {
                throw err;
            }
            this.#mysqlConnectionPool = await this.#fastify.mysql.pool;
            this.#queryJobsTableName = config.queryJobsTableName;
        });
    }

    /**
     * Initializes MongoDB connection.
     *
     * @param {object} config
     * @param {string} config.host
     * @param {number} config.port
     * @param {string} config.database
     * @param {string} config.irFilesCollectionName
     */
    #initMongo (config) {
        this.#fastify.register(fastifyMongo, {
            forceClose: true,
            url: `mongodb://${config.host}:${config.port}/${config.database}`,
        }).after((err) => {
            if (err) {
                throw err;
            }
            this.irFilesCollection =
                this.#fastify.mongo.db.collection(config.irFilesCollectionName);
        });
    }
}

export default fastifyPlugin(async (app, options) => {
    await app.decorate("dbManager", new DbManager(app, options));
});
