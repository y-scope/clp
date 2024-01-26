import mysql from "mysql2/promise";

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
export const initSQL = async () => {
    const CLP_DB_HOST = process.env['CLP_DB_HOST'];
    const CLP_DB_PORT = process.env['CLP_DB_PORT'];
    const CLP_DB_NAME = process.env['CLP_DB_NAME'];
    const CLP_DB_USER = process.env['CLP_DB_USER'];
    const CLP_DB_PASS = process.env['CLP_DB_PASS'];
    if ([CLP_DB_HOST, CLP_DB_PORT, CLP_DB_NAME, CLP_DB_USER, CLP_DB_PASS].includes(undefined)) {
        console.error("Environment variables CLP_DB_URL, CLP_DB_USER and CLP_DB_PASS need to be defined");
        process.exit(1);
    }

    SQL_CONNECTION = await mysql.createConnection({
        host: CLP_DB_HOST,
        port: parseInt(CLP_DB_PORT),
        database: CLP_DB_NAME,
        user: CLP_DB_USER,
        password: CLP_DB_PASS,
    });
    await SQL_CONNECTION.connect();
}

/**
 * Deinitializes the SQL database connection if it is initialized.
 */
export const deinitSQL = async () => {
    if (null !== SQL_CONNECTION) {
        await SQL_CONNECTION.end();
        SQL_CONNECTION = null;
    }
}
