import mysql from "mysql2/promise";

import {
    deinitCompressionDbManager,
    deinitStatsDbManager,
    initCompressionDbManager,
    initStatsDbManager,
} from "/imports/api/ingestion/server/publications";
import {initSearchJobsDbManager} from "/imports/api/search/server/methods";
import {logger} from "/imports/utils/logger";


const DB_CONNECTION_LIMIT = 2;
const DB_MAX_IDLE = DB_CONNECTION_LIMIT;
const DB_IDLE_TIMEOUT_IN_MS = 10000;

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
 * @param {string} dbConfig.dbUser
 * @param {string} dbConfig.dbPassword
 *
 * @param {object} tableNames
 * @param {string} tableNames.clpArchivesTableName
 * @param {string} tableNames.clpFilesTableName
 * @param {string} tableNames.compressionJobsTableName
 * @param {string} tableNames.searchJobsTableName
 *
 * @returns {Promise<void>}
 * @throws {Error} on error.
 */
const initDbManagers = async ({
    dbHost,
    dbPort,
    dbName,
    dbUser,
    dbPassword,
}, {
    clpArchivesTableName,
    clpFilesTableName,
    compressionJobsTableName,
    searchJobsTableName,
}) => {
    if (null !== dbConnPool) {
        throw Error("This method should not be called twice.");
    }

    try {
        dbConnPool = await mysql.createPool({
            host: dbHost,
            port: dbPort,
            database: dbName,
            user: dbUser,
            password: dbPassword,
            bigNumberStrings: true,
            supportBigNumbers: true,
            enableKeepAlive: true,
            connectionLimit: DB_CONNECTION_LIMIT,
            maxIdle: DB_MAX_IDLE,
            idleTimeout: DB_IDLE_TIMEOUT_IN_MS,
            timezone: "Z",
        });

        initStatsDbManager(dbConnPool, {
            clpArchivesTableName,
            clpFilesTableName,
        });
        initCompressionDbManager(dbConnPool, {
            compressionJobsTableName,
        });
        initSearchJobsDbManager(dbConnPool, {
            searchJobsTableName,
        });
    } catch (e) {
        logger.error("Unable to create MySQL / mariadb connection pool.", e.toString());
        throw e;
    }
};

/**
 * De-initialize database managers.
 * @returns {Promise<void>}
 * @throws {Error} on error.
 */
const deinitDbManagers = async () => {
    deinitStatsDbManager();
    deinitCompressionDbManager();

    await dbConnPool.end();
};

export {
    deinitDbManagers,
    initDbManagers,
};
