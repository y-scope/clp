import mysql from "mysql2/promise";
import msgpack from "@msgpack/msgpack";
import {JOB_STATUS_WAITING_STATES, JobStatus} from "../constants";
import {sleep} from "../../../utils/misc";

const SEARCH_JOBS_TABLE_COLUMN_NAMES = {
    ID: "id",
    STATUS: "status",
    SEARCH_CONFIG: "search_config",
};

/**
 * Class for submitting and monitoring search jobs in the database.
 */
class SearchJobsDbManager {
    #sqlDbConnection;
    #searchJobsTableName;

    /**
     * Creates a new instance.
     * @param {string} dbHost
     * @param {number} dbPort
     * @param {string} dbName
     * @param {string} dbUser
     * @param {string} dbPassword
     * @param {string} searchJobsTableName
     * @returns {Promise<SearchJobsDbManager>}
     * @throws {Error} on error.
     */
    static async createNew({dbHost, dbPort, dbName, dbUser, dbPassword, searchJobsTableName}) {
        const conn = await mysql.createConnection({
            host: dbHost,
            port: dbPort,
            database: dbName,
            user: dbUser,
            password: dbPassword,
        });
        return new SearchJobsDbManager(conn, searchJobsTableName);
    }

    /**
     * @param {mysql.Connection} sqlDbConnection
     * @param {string} searchJobsTableName
     */
    constructor(sqlDbConnection, searchJobsTableName) {
        this.#sqlDbConnection = sqlDbConnection;
        this.#searchJobsTableName = searchJobsTableName;
    }

    /**
     * Connects to the database.
     * @returns {Promise<void>}
     * @throws {Error} on error.
     */
    async connect() {
        await this.#sqlDbConnection.connect();
    }

    /**
     * Disconnects from the database.
     * @returns {Promise<void>}
     * @throws {Error} on error.
     */
    async disconnect() {
        await this.#sqlDbConnection.end();
    }

    /**
     * Submits a query job to the database.
     * @param {Object} searchConfig The arguments for the query.
     * @returns {Promise<number>} The job's ID.
     * @throws {Error} on error.
     */
    async submitQuery(searchConfig) {
        const [queryInsertResults] = await this.#sqlDbConnection.query(
            `INSERT INTO ${this.#searchJobsTableName}
                    (${SEARCH_JOBS_TABLE_COLUMN_NAMES.SEARCH_CONFIG})
                    VALUES (?)`,
            [Buffer.from(msgpack.encode(searchConfig))],
        );
        return queryInsertResults.insertId;
    }

    /**
     * Submits a query cancellation request to the database.
     * @param {number} jobId ID of the job to cancel.
     * @returns {Promise<void>}
     * @throws {Error} on error.
     */
    async submitQueryCancellation(jobId) {
        await this.#sqlDbConnection.query(
            `UPDATE ${this.#searchJobsTableName}
                    SET ${SEARCH_JOBS_TABLE_COLUMN_NAMES.STATUS} = ${JobStatus.CANCELLING}
                    WHERE ${SEARCH_JOBS_TABLE_COLUMN_NAMES.ID} = ?`,
            jobId,
        );
    }

    /**
     * Waits for the job to complete.
     * @param {number} jobId
     * @returns {Promise<void>}
     * @throws {Error} on MySQL error, if the job wasn't found in the database, if the job was
     * cancelled, or if the job completed in an unexpected state.
     */
    async awaitJobCompletion(jobId) {
        while (true) {
            let rows;
            try {
                const [queryRows, _] = await this.#sqlDbConnection.query(
                    `SELECT ${SEARCH_JOBS_TABLE_COLUMN_NAMES.STATUS}
                        FROM ${this.#searchJobsTableName}
                        WHERE ${SEARCH_JOBS_TABLE_COLUMN_NAMES.ID} = ?`,
                    jobId,
                );
                rows = queryRows;
            } catch (e) {
                throw new Error(`Failed to query status for job ${jobId} - ${e}`);
            }
            if (rows.length < 1) {
                throw new Error(`Job ${jobId} not found in database.`);
            }
            const status = rows[0][SEARCH_JOBS_TABLE_COLUMN_NAMES.STATUS];

            if (false === JOB_STATUS_WAITING_STATES.includes(status)) {
                if (JobStatus.CANCELLED === status) {
                    throw new Error(`Job ${jobId} was cancelled.`);
                } else if (JobStatus.SUCCESS !== status) {
                    throw new Error(`Job ${jobId} exited with unexpected status=${status}: `
                            `${Object.keys(JobStatus)[status]}.`);
                }
                break;
            }

            await sleep(0.5);
        }
    }
}

export default SearchJobsDbManager;
