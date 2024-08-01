import mysql from "mysql2/promise";

import {
    deinitCompressionDbManager,
    deinitStatsDbManager,
    initCompressionDbManager,
    initStatsDbManager,
} from "/imports/api/ingestion/server/publications";
import {initQueryJobsDbManager} from "/imports/api/search/server/methods";
import {logger} from "/imports/utils/logger";


const DB_CONNECTION_LIMIT = 2;
const DB_MAX_IDLE = DB_CONNECTION_LIMIT;
const DB_IDLE_TIMEOUT_MILLIS = 10000;

/**
 * @type {import("mysql2/promise").Pool|null}
 */
let dbConnPool = null;

/**
 * Creates a new database connection and initializes DB managers with it.
 *
 * @param {object} dbConfig
 * @param {string} dbConfig.dbHost
 * @param {number} dbConfig.dbPort
 * @param {string} dbConfig.dbName
 * @param {string} dbConfig.dbPassword
 * @param {string} dbConfig.dbUser
 * @param {object} tableNames
 * @param {string} tableNames.clpArchivesTableName
 * @param {string} tableNames.clpFilesTableName
 * @param {string} tableNames.compressionJobsTableName
 * @param {string} tableNames.queryJobsTableName
 * @return {Promise<void>}
 * @throws {Error} on error.
 */
const initDbManagers = async ({
    dbHost,
    dbPort,
    dbName,
    dbPassword,
    dbUser,
}, {
    clpArchivesTableName,
    clpFilesTableName,
    compressionJobsTableName,
    queryJobsTableName,
}) => {
    if (null !== dbConnPool) {
        throw Error("This method should not be called twice.");
    }

    try {
        // This method shall not be called twice and therefore incurs no race condition.
        // eslint-disable-next-line require-atomic-updates
        dbConnPool = await mysql.createPool({
            host: dbHost,
            port: dbPort,

            database: dbName,
            password: dbPassword,
            user: dbUser,

            bigNumberStrings: true,
            supportBigNumbers: true,
            timezone: "Z",

            connectionLimit: DB_CONNECTION_LIMIT,
            enableKeepAlive: true,
            idleTimeout: DB_IDLE_TIMEOUT_MILLIS,
            maxIdle: DB_MAX_IDLE,
        });

        initCompressionDbManager(dbConnPool, {
            compressionJobsTableName,
        });
        initQueryJobsDbManager(dbConnPool, {
            queryJobsTableName,
        });
        initStatsDbManager(dbConnPool, {
            clpArchivesTableName,
            clpFilesTableName,
        });
    } catch (e) {
        logger.error("Unable to create MySQL / mariadb connection pool.", e.toString());
        throw e;
    }
};

/**
 * De-initializes database managers.
 *
 * @return {Promise<void>}
 * @throws {Error} on error.
 */
const deinitDbManagers = async () => {
    deinitCompressionDbManager();
    deinitStatsDbManager();

    await dbConnPool.end();
};

export {
    deinitDbManagers,
    initDbManagers,
};
