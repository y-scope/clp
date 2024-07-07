import fastifyPlugin from "fastify-plugin";

import fastifyMysql from "@fastify/mysql";
import fastifyMongo from "@fastify/mongodb";
import msgpack from "@msgpack/msgpack";


/**
 * Class representing the database manager.
 */
class DbManager {
    /**
     * Creates a DbManager.
     *
     * @param {import("fastify").FastifyInstance} app - The Fastify application instance
     * @param {Object} dbConfig - The database configuration
     * @param {Object} dbConfig.mysqlConfig - The MySQL configuration
     * @param {Object} dbConfig.mongoConfig - The MongoDB configuration
     */
    constructor(app, dbConfig) {
        this.app = app;
        this.initMySql(dbConfig.mysqlConfig);
        this.initMongo(dbConfig.mongoConfig);
    }

    /**
     * Initializes MySQL connection.
     *
     * @param {Object} config
     * @param {string} config.user
     * @param {string} config.password
     * @param {string} config.host
     * @param {number} config.port
     * @param {string} config.database
     * @param {string} config.queryJobsTableName
     */
    initMySql(config) {
        this.app.register(fastifyMysql, {
            promise: true,
            connectionString: `mysql://${config.user}:${config.password}@${config.host}:` +
                `${config.port}/${config.database}`,
        }).after(async (err) => {
            if (err) {
                throw err;
            }
            this.mysqlConnection = await this.app.mysql.getConnection();
            this.queryJobsTableName = config.queryJobsTableName;
        });
    }

    /**
     * Initializes MongoDB connection.
     *
     * @param {Object} config
     * @param {string} config.host
     * @param {number} config.port
     * @param {string} config.database
     * @param {string} config.statsCollectionName
     */
    initMongo(config) {
        this.app.register(fastifyMongo, {
            forceClose: true,
            url: `mongodb://${config.host}:${config.port}/${config.database}`,
        }).after(err => {
            if (err) {
                throw err;
            }
            this.mongoStatsCollection = this.app.mongo.db.collection(config.statsCollectionName);
        });
    }

    /**
     * Inserts a decompression job into MySQL.
     *
     * @param {Object} jobConfig - The job configuration.
     * @returns {Promise<Object>} The result of the insert query or null if an error occurred.
     */
    async insertDecompressionJob(jobConfig) {
        return await this.mysqlConnection.query(
            `INSERT INTO ${this.queryJobsTableName} (id, job_config)
             VALUES (?, ?)`,
            [
                1,
                Buffer.from(msgpack.encode(jobConfig)),
            ]
        );
    }

    /**
     * Retrieves a decompression job from MySQL.
     *
     * @param {number} jobId - The ID of the job.
     * @returns {Promise<Object>} The job configuration.
     */
    async getDecompressionJob(jobId) {
        const [results] = await this.mysqlConnection.query(
            `SELECT job_config
             FROM ${this.queryJobsTableName}
             WHERE id = ?`,
            [jobId],
        );

        return msgpack.decode(results[0].job_config);
    }

    /**
     * Retrieve statistics from MongoDB.
     *
     * @returns {Promise<Array>} The array of statistics documents.
     */
    async getStats() {
        return await this.mongoStatsCollection.find().toArray();
    }
}

export default fastifyPlugin(async (app, options) => {
    await app.decorate("dbManager", new DbManager(app, options));
});
