const fastifyMongo = require("@fastify/mongodb");
const fastifyMysql = require("@fastify/mysql");
const fastifyPlugin = require("fastify-plugin");
const msgpack = require("@msgpack/msgpack");


class DbManager {
    #app;

    constructor (app, options) {
        this.#app = app;

        this.initMongo(options.mongo);
        this.initMySQL(options.mysql);
    }

    initMongo (mongoOptions) {
        this.#app.register(fastifyMongo, {
            forceClose: true,
            url: mongoOptions.url,
        }).after((err) => {
            if (err) throw err;
            this.mongoStatsCollection = this.#app.mongo.db.collection("stats");
        });
    }

    initMySQL (mysqlOptions) {
        this.#app.register(fastifyMysql, {
            promise: true,
            connectionString: mysqlOptions.connectionString,
        }).after(async (err) => {
            if (err) throw err;

            // TODO: revisit potential connection idle issue due to `wait_timeout`
            this.mysqlConnection = await this.#app.mysql.getConnection();
        });
    }

    async getStatsFromMongo () {
        return this.mongoStatsCollection.find().toArray();
    }

    async getSearchJobsFromMySQL () {
        const [rows] = await this.mysqlConnection.query("SELECT * FROM search_jobs");
        return rows;
    }

    async createSearchJobInMySQL (queryString) {
        return await this.mysqlConnection.query(
            "INSERT INTO search_jobs (search_config) VALUES (?)",
            [
                Buffer.from(msgpack.encode({
                    begin_timestamp: 0,
                    end_timestamp: Number.MAX_SAFE_INTEGER,
                    ignore_case: true,
                    max_num_results: 1000,
                    query_string: queryString,
                })),
            ]
        );
    }
}

module.exports = fastifyPlugin(async (app, options) => {
    await app.decorate("dbManager", new DbManager(app, options));
});
