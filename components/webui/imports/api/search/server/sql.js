import msgpack from "@msgpack/msgpack";
import mysql from "mysql2/promise";

import {logger} from "/imports/utils/logger";
import {sleep} from "../../../utils/utils";
import {JOB_STATUS_WAITING_STATES, JobStatus} from "../constants";

const SEARCH_JOBS_TABLE_NAME = "distributed_search_jobs";
const SEARCH_JOBS_TABLE_COLUMN_NAMES = {
    ID: "id",
    STATUS: "status",
    STATUS_MSG: "status_msg",
    CREATION_TIME: "creation_time",
    START_TIME: "start_time",
    DURATION: "duration",
    REDUCER_HOST:  "reducer_host",
    REDUCER_PORT:  "reducer_port",
    SEARCH_CONFIG:  "search_config",
};

/**
 * SQL connection for submitting queries to the backend
 * @type {mysql.Connection}
 */
export let SQL_CONNECTION = null;

/**
 * Initializes the SQL database connection using environment variables.
 *
 * @throws {Error} if unable to connect to the SQL database.
 */
export const initSql = async (host, port, database, user, password) => {
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
            `host=${host}, port=${port}, database=${database}, user=${user}: ` +
            `error=${e}`);
        throw e;
    }
};

/**
 * De-initializes the SQL database connection if it is initialized.
 */
export const deinitSql = async () => {
    if (null !== SQL_CONNECTION) {
        await SQL_CONNECTION.end();
        SQL_CONNECTION = null;
    }
};

/**
 * Submits a query job to the SQL database and returns the job ID.
 *
 * @param {Object} args containing the search configuration.
 * @returns {number|null} job ID on successful submission, or null in case of failure
 */
export const submitQuery = async (args) => {
    let jobId = null;

    try {
        const [queryInsertResults] = await SQL_CONNECTION.query(
            `INSERT INTO ${SEARCH_JOBS_TABLE_NAME}
                 (search_config)
             VALUES (?) `, [Buffer.from(msgpack.encode(args))],
        );
        jobId = queryInsertResults.insertId;
    } catch (e) {
        logger.error("Unable to submit query job to SQL DB", e);
    }

    return jobId;
};

/**
 * Waits for a job to finish and retrieves its status from the database.
 *
 * @param {number} jobId of the job to monitor
 *
 * @returns {?string} null if the job completes successfully; an error message if the job exits
 * in an unexpected status or encounters an error during monitoring
 */
export const waitTillJobFinishes = async (jobId) => {
    let errorMsg = null;

    try {
        while (true) {
            const [rows, _] = await SQL_CONNECTION.query(`SELECT status
                                                          FROM ${SEARCH_JOBS_TABLE_NAME}
                                                          WHERE id = ${jobId}`);
            const status = rows[0][SEARCH_JOBS_TABLE_COLUMN_NAMES.STATUS];

            if (false === JOB_STATUS_WAITING_STATES.includes(status)) {
                logger.info(`Job ${jobId} exited with status = ${status}: ${Object.keys(
                        JobStatus)[status]}.`)

                if (JobStatus.CANCELLED === status) {
                  errorMsg = `Job was cancelled.`
                } else if (JobStatus.SUCCESS !== status) {
                    errorMsg = `Job exited with unexpected status=${status}: ${Object.keys(
                        JobStatus)[status]}.`;
                }

                break;
            }

            await sleep(0.5);
        }
    } catch (e) {
        errorMsg = `Error querying job status for jobId=${jobId}: ${e}`;
        logger.error(errorMsg);
    }

    return errorMsg;
};

/**
 * Cancels a job by updating its status to 'CANCELLING' in the database.
 *
 * @param {string} jobId of the job to be cancelled
 */
export const cancelQuery = async (jobId) => {
    await SQL_CONNECTION.query(`UPDATE ${SEARCH_JOBS_TABLE_NAME}
                                SET status = ${JobStatus.CANCELLING}
                                WHERE id = (?)`, [jobId]);
};