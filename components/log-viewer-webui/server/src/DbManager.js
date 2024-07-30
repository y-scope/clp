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
 * List of states that indicate the job is either pending or in progress.
 */
const QUERY_JOB_STATUS_WAITING_STATES = Object.freeze([
    QUERY_JOB_STATUS.PENDING,
    QUERY_JOB_STATUS.RUNNING,
    QUERY_JOB_STATUS.CANCELLING,
]);

/* eslint-disable sort-keys */
let enumQueryType;
/**
 * Enum of job types, matching the `QueryJobType` class in
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
 * Class to manage connections to the jobs database (MySQL) and results cache (MongoDB).
 */
class DbManager {
    /**
     * @type {import("fastify").FastifyInstance |
     * {mysql: import("@fastify/mysql").MySQLPromisePool} |
     * {mongo: import("@fastify/mongodb").FastifyMongoObject}}
     */
    #fastify;

    /**
     * @type {import("@fastify/mysql").PromisePool}
     */
    #mysqlConnectionPool;

    /**
     * @type {import("mongodb").Collection}
     */
    #irFilesCollection;

    #queryJobsTableName;

    /**
     * @param {import("fastify").FastifyInstance} app
     * @param {object} dbConfig
     * @param {object} dbConfig.mysqlConfig
     * @param {object} dbConfig.mongoConfig
     */
    constructor (app, dbConfig) {
        this.#fastify = app;
        this.#initMySql(dbConfig.mysqlConfig);
        this.#initMongo(dbConfig.mongoConfig);
    }

    /**
     * Submits an IR extraction job to the scheduler and waits for it to finish.
     *
     * @param {object} jobConfig
     * @return {Promise<number|null>} The ID of the job or null if an error occurred.
     */
    async submitAndWaitForExtractIrJob (jobConfig) {
        let jobId;
        try {
            const [result] = await this.#mysqlConnectionPool.query(
                `INSERT INTO ${this.#queryJobsTableName} (job_config, type)
             VALUES (?, ?)`,
                [
                    Buffer.from(msgpack.encode(jobConfig)),
                    QUERY_JOB_TYPE.EXTRACT_IR,
                ]
            );

            ({insertId: jobId} = result);
            await this.#awaitJobCompletion(jobId);
        } catch (e) {
            this.#fastify.log.error(e);

            return null;
        }

        return jobId;
    }

    /**
     * Gets the metadata for an IR file extracted from part of an original file, where the original
     * file has the given ID and the extracted part contains the given log event index.
     *
     * @param {string} origFileId
     * @param {number} logEventIdx
     * @return {Promise<object>} A promise that resolves to the extracted IR file's metadata.
     */
    async getExtractedIrFileMetadata (origFileId, logEventIdx) {
        return await this.#irFilesCollection.findOne({
            orig_file_id: origFileId,
            begin_msg_ix: {$lte: logEventIdx},
            end_msg_ix: {$gt: logEventIdx},
        });
    }

    /**
     * Initializes the MySQL plugin.
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
            this.#mysqlConnectionPool = this.#fastify.mysql.pool;
            this.#queryJobsTableName = config.queryJobsTableName;
        });
    }

    /**
     * Initializes the MongoDB plugin.
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
            this.#irFilesCollection =
                this.#fastify.mongo.db.collection(config.irFilesCollectionName);
        });
    }

    /**
     * Waits for the job with the given ID to finish.
     *
     * @param {number} jobId
     * @throws {Error} If there's an error querying the job's status, the job is not found in the
     * database, the job was cancelled, or it exited with an unexpected status.
     */
    async #awaitJobCompletion (jobId) {
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
}

export default fastifyPlugin(async (app, options) => {
    await app.decorate("dbManager", new DbManager(app, options));
});
