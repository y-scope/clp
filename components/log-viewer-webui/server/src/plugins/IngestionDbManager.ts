import mysql from "mysql2/promise";

import {
    deinitCompressionDbManager,
    deinitStatsDbManager,
    initCompressionDbManager,
    initStatsDbManager,
} from "./publications.js";


const DB_CONNECTION_LIMIT = 2;
const DB_MAX_IDLE = DB_CONNECTION_LIMIT;
const DB_IDLE_TIMEOUT_MILLIS = 10000;

interface DatabaseConfig {
    dbHost: string;
    dbPort: number;
    dbName: string;
    dbPassword: string;
    dbUser: string;
}

interface TableNames {
    clpArchivesTableName: string;
    clpFilesTableName: string;
    compressionJobsTableName: string;
    queryJobsTableName: string;
}

let dbConnPool: mysql.Pool | null = null;

/**
 * Creates a new database connection and initializes DB managers with it.
 *
 * @param dbConfig
 * @param dbConfig.dbHost
 * @param dbConfig.dbPort
 * @param dbConfig.dbName
 * @param dbConfig.dbPassword
 * @param dbConfig.dbUser
 * @param tableNames
 * @param tableNames.clpArchivesTableName
 * @param tableNames.clpFilesTableName
 * @param tableNames.compressionJobsTableName
 * @param tableNames.queryJobsTableName
 * @throws {Error} on error.
 */
const initDbManagers = async (
    dbConfig: DatabaseConfig,
    tableNames: TableNames
) => {
    if (null !== dbConnPool) {
        throw Error("This method should not be called twice.");
    }

    try {
        dbConnPool = mysql.createPool({
            host: dbConfig.dbHost,
            port: dbConfig.dbPort,

            database: dbConfig.dbName,
            password: dbConfig.dbPassword,
            user: dbConfig.dbUser,

            bigNumberStrings: true,
            supportBigNumbers: true,
            timezone: "Z",

            connectionLimit: DB_CONNECTION_LIMIT,
            enableKeepAlive: true,
            idleTimeout: DB_IDLE_TIMEOUT_MILLIS,
            maxIdle: DB_MAX_IDLE,
        });

        initCompressionDbManager(dbConnPool, tableNames.compressionJobsTableName);
        initStatsDbManager(dbConnPool, {
            clpArchivesTableName: tableNames.clpArchivesTableName,
            clpFilesTableName: tableNames.clpFilesTableName,
        });
    } catch (e) {
        console.error("Unable to create MySQL / mariadb connection pool.", e.toString());
        throw e;
    }
};

/**
 * De-initializes database managers.
 *
 * @throws {Error} on error.
 */
const deinitDbManagers = async () => {
    deinitCompressionDbManager();
    deinitStatsDbManager();

    if (dbConnPool) {
        await dbConnPool.end();
    }
};

export {
    deinitDbManagers,
    initDbManagers,
};
