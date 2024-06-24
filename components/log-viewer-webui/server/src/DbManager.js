import fastifyPlugin from "fastify-plugin";

import fastifyMysql from "@fastify/mysql";
import fastifyMongo from "@fastify/mongodb";
import msgpack from "@msgpack/msgpack";


class DbManager {
    constructor(app, dbConfig) {
        this.app = app;
        this.queryJobsTableName = dbConfig.mysqlConfig.queryJobsTableName;
        this.initMySql(dbConfig.mysqlConfig);
        this.initMongo(dbConfig.mongoConfig);
    }

    initMySql(config) {
       console.log(`mysql://${config.user}:${config.password}@${config.host}:` + `${config.port}/${config.database}`)
        this.app.register(fastifyMysql, {
            promise: true,
            connectionString: `mysql://${config.user}:${config.password}@${config.host}:` +
                `${config.port}/${config.database}`,
        }).after(async (err) => {
            if (err) {
                throw err;
            }
            this.mysqlConnection = await this.app.mysql.getConnection();
        });
    }

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

    async insertDecompressionJob(jobConfig) {
        try {
            return await this.mysqlConnection.query(
                `INSERT INTO ${this.queryJobsTableName} (id, job_config)
                 VALUES (?, ?)`,
                [1,
                    Buffer.from(msgpack.encode(jobConfig))]
            );
        } catch (e) {
            console.error(e);
            return null;
        }

    }

    async getDecompressionJob(jobId) {
        const [results] = await this.mysqlConnection.query(
            `SELECT job_config
             FROM ${this.queryJobsTableName}
             WHERE id = ?`,
            [jobId],
        );

        return msgpack.decode(results[0].job_config);
    }

    async getStats() {
        return await this.mongoStatsCollection.find().toArray();
    }
}

export default fastifyPlugin(async (app, options) => {
    await app.decorate("dbManager", new DbManager(app, options));
});
