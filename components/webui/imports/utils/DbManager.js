import {logger} from "/imports/utils/logger";
import mysql from "mysql2/promise";

import {deinitStatsDbManager, initStatsDbManager} from "../api/ingestion/server/publications";
import {initSearchJobsDbManager} from "../api/search/server/methods";


/**
 * @type {mysql.Connection|null}
 */
let dbConnection = null;

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
 * @param {string} tableNames.searchJobsTableName
 * @param {string} tableNames.clpArchivesTableName
 * @param {string} tableNames.clpFilesTableName
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
    searchJobsTableName,
    clpArchivesTableName,
    clpFilesTableName,
}) => {
    if (null !== dbConnection) {
        logger.error("This method should not be called twice.");
        return;
    }

    try {
        dbConnection = await mysql.createConnection({
            host: dbHost,
            port: dbPort,
            database: dbName,
            user: dbUser,
            password: dbPassword,
            bigNumberStrings: true,
            supportBigNumbers: true,
        });
        await dbConnection.connect();

        initSearchJobsDbManager(dbConnection, {
            searchJobsTableName,
        });
        initStatsDbManager(dbConnection, {
            clpArchivesTableName,
            clpFilesTableName,
        });
    } catch (e) {
        logger.error("Unable to create MySQL / mariadb connection.", e.toString());
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

    await dbConnection.end();
};

export {initDbManagers, deinitDbManagers};
