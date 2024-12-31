import fastifyPlugin from "fastify-plugin";


import {Collection} from "mongodb";
import fastifyMongo from "@fastify/mongodb";
import fastifyMysql, {MySQLPromisePool} from "@fastify/mysql";
import {encode as msgpackEncode} from "@msgpack/msgpack";

import {sleep} from "./utils.js";
import {FastifyInstance, FastifyPluginCallback} from "fastify";
import {Pool as PromisePool, RowDataPacket, ResultSetHeader} from "mysql2/promise";


declare module 'fastify' {
    interface FastifyInstance {
        // The typing of `@fastify/mysql` needs to be manually specified.
        // See https://github.com/fastify/fastify-mysql#typescript
        mysql: MySQLPromisePool

        dbManager: DbManager
    }
}

interface DbManagerOptions {
    mysqlConfig: {
        user: string,
        password: string,
        host: string,
        port: number,
        database: string,
        queryJobsTableName: string,
    },
    mongoConfig: {
        database: string,
        host: string,
        streamFilesCollectionName: string,
        port: number,
    }
}

interface StreamFileMongoDocument {
    path: string;
    stream_id: string;
    begin_msg_ix: number;
    end_msg_ix: number;
    is_last_chunk: boolean;
}

type StreamFilesCollection = Collection<StreamFileMongoDocument>;


/**
 * Interval in milliseconds for polling the completion status of a job.
 */
const JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS = 0.5;

/**
 * The `query_jobs` table's column names.
 *
 * @enum {string}
 */
enum QUERY_JOBS_TABLE_COLUMN_NAMES {
    ID = "id",
    STATUS = "status",
    TYPE = "type",
    JOB_CONFIG = "job_config",
}

/**
 * Matching the `QueryJobStatus` class in
 * `job_orchestration.query_scheduler.constants`.
 *
 * @enum {number}
 */
enum QUERY_JOB_STATUS {
    PENDING = 0,
    RUNNING,
    SUCCEEDED,
    FAILED,
    CANCELLING,
    CANCELLED,
}

/**
 * List of states that indicate the job is either pending or in progress.
 */
const QUERY_JOB_STATUS_WAITING_STATES = Object.freeze([
    QUERY_JOB_STATUS.PENDING,
    QUERY_JOB_STATUS.RUNNING,
    QUERY_JOB_STATUS.CANCELLING,
]);

/**
 * Matching the `QueryJobType` class in `job_orchestration.query_scheduler.constants`.
 */
enum QUERY_JOB_TYPE {
    SEARCH_OR_AGGREGATION = 0,
    EXTRACT_IR,
    EXTRACT_JSON,
}

/**
 * List of valid extract job types.
 */
const EXTRACT_JOB_TYPES = Object.freeze([
    QUERY_JOB_TYPE.EXTRACT_IR,
    QUERY_JOB_TYPE.EXTRACT_JSON,
]);

/**
 * Class to manage connections to the jobs database (MySQL) and results cache (MongoDB).
 */
class DbManager {
    #fastify;

    readonly #mysqlConnectionPool: PromisePool;

    readonly #streamFilesCollection: StreamFilesCollection;

    readonly #queryJobsTableName: string;

    static async create(app: FastifyInstance, dbConfig: DbManagerOptions) {
        const mysqlConnectionPool = await DbManager.#initMySql(app, dbConfig.mysqlConfig);
        const {streamFilesCollection} = await DbManager.#initMongo(app, dbConfig.mongoConfig);
        return new DbManager({
            app: app,
            mysqlConnectionPool: mysqlConnectionPool,
            streamFilesCollection: streamFilesCollection,
            queryJobsTableName: dbConfig.mysqlConfig.queryJobsTableName
        });
    }

    /**
     */
    constructor({app, mysqlConnectionPool, queryJobsTableName, streamFilesCollection}: {
        app: FastifyInstance,
        mysqlConnectionPool: PromisePool,
        queryJobsTableName: string
        streamFilesCollection: StreamFilesCollection,
    }) {
        this.#fastify = app;

        this.#mysqlConnectionPool = mysqlConnectionPool;
        this.#streamFilesCollection = streamFilesCollection;

        this.#queryJobsTableName = queryJobsTableName;
    }

    /**
     * Submits a stream extraction job to the scheduler and waits for it to finish.
     *
     * @param props
     * @param {number} props.jobType
     * @param {number} props.logEventIdx
     * @param {string} props.streamId
     * @param {number} props.targetUncompressedSize
     * @return {Promise<number|null>} The ID of the job or null if an error occurred.
     */
    async submitAndWaitForExtractStreamJob({
                                               jobType,
                                               logEventIdx,
                                               streamId,
                                               targetUncompressedSize,
                                           }: {
        jobType: number,
        logEventIdx: number,
        streamId: string,
        targetUncompressedSize: number,
    }): Promise<number | null> {
        let jobConfig;
        if (QUERY_JOB_TYPE.EXTRACT_IR === jobType) {
            jobConfig = {
                file_split_id: null,
                msg_ix: logEventIdx,
                orig_file_id: streamId,
                target_uncompressed_size: targetUncompressedSize,
            };
        } else if (QUERY_JOB_TYPE.EXTRACT_JSON === jobType) {
            jobConfig = {
                archive_id: streamId,
                target_chunk_size: targetUncompressedSize,
            };
        }

        let jobId;
        try {
            const [result] = await this.#mysqlConnectionPool.query<ResultSetHeader>(
                `INSERT INTO ${this.#queryJobsTableName} (job_config, type)
                 VALUES (?, ?)`,
                [
                    Buffer.from(msgpackEncode(jobConfig)),
                    jobType,
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
     * Gets the metadata for the extracted stream that has the given streamId and contains the
     * given logEventIdx.
     *
     * @param {string} streamId
     * @param {number} logEventIdx
     * @return {Promise<object>} A promise that resolves to the extracted stream's metadata.
     */
    async getExtractedStreamFileMetadata(streamId: string, logEventIdx: number): Promise<StreamFileMongoDocument | null> {
        return await this.#streamFilesCollection.findOne({
            stream_id: streamId,
            begin_msg_ix: {$lte: logEventIdx},
            end_msg_ix: {$gt: logEventIdx},
        });
    }

    /**
     * Initializes the MySQL plugin.
     */
    static async #initMySql(app: FastifyInstance, config: DbManagerOptions['mysqlConfig']) {
        await app.register(fastifyMysql, {
            promise: true,
            connectionString: `mysql://${config.user}:${config.password}@${config.host}:` +
                `${config.port}/${config.database}`,
        })

        return app.mysql.pool
    }

    /**
     * Initializes the MongoDB plugin.
     */
    static async #initMongo(app: FastifyInstance, config: DbManagerOptions['mongoConfig']): Promise<{
        streamFilesCollection: StreamFilesCollection,
    }> {
        await app.register(fastifyMongo, {
            forceClose: true,
            url: `mongodb://${config.host}:${config.port}/${config.database}`,
        })
        if ("undefined" === typeof app.mongo.db) {
            throw new Error("Failed to initialize MongoDB plugin.");
        }

        return {streamFilesCollection: app.mongo.db.collection(config.streamFilesCollectionName)};
    }

    /**
     * Waits for the job with the given ID to finish.
     *
     * @param {number} jobId
     * @throws {Error} If there's an error querying the job's status, the job is not found in the
     * database, the job was cancelled, or it exited with an unexpected status.
     */
    async #awaitJobCompletion(jobId: number) {
        while (true) {
            let rows: RowDataPacket[];
            try {
                const [queryResult] = await this.#mysqlConnectionPool.query<RowDataPacket[]>(
                    `
                        SELECT ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS}
                        FROM ${this.#queryJobsTableName}
                        WHERE ${QUERY_JOBS_TABLE_COLUMN_NAMES.ID} = ?
                    `,
                    jobId,
                );

                rows = queryResult;
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

const dbManagerPluginCallback: FastifyPluginCallback<DbManagerOptions> = async (app, options) => {
    const dbManager = await DbManager.create(app, options);
    app.decorate("dbManager", dbManager);
}

export {
    EXTRACT_JOB_TYPES,
    QUERY_JOB_TYPE,
};
export default fastifyPlugin(dbManagerPluginCallback);
