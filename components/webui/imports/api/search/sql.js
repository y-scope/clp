import mysql from "mysql2/promise";
import {logger} from "/imports/utils/logger";

// SQL connection for submitting queries to the backend
export let SQL_CONNECTION = null;

/**
 * Initializes the SQL database connection using environment variables.
 *
 * This function sets up a connection to a SQL database using environment variables.
 * It requires the following environment variables to be defined:
 * - CLP_DB_HOST: The host address of the SQL database.
 * - CLP_DB_PORT: The port number for the SQL database.
 * - CLP_DB_NAME: The name of the SQL database.
 * - CLP_DB_USER: The username for authenticating with the SQL database.
 * - CLP_DB_PASS: The password for authenticating with the SQL database.
 *
 * @throws {Error} Throws an error and exits the process if any required environment variables are undefined.
 */
export const initSQL = async (host, port, database, user, password) => {
    try {
        SQL_CONNECTION = await mysql.createConnection({
            host: host,
            port: port,
            database: database,
            user: user,
            password: password,
        });
        await SQL_CONNECTION.connect();
    } catch (e) {
        logger.error(`Unable to create MySQL / mariadb connection with ` +
            `host=${host}, port=${port}, database=${database}, user=${user}`);
    }
};

/**
 * De-initializes the SQL database connection if it is initialized.
 */
export const deinitSQL = async () => {
    if (null !== SQL_CONNECTION) {
        await SQL_CONNECTION.end();
        SQL_CONNECTION = null;
    }
};
