import msgpack from "@msgpack/msgpack";

import {sleep} from "/imports/utils/misc";

import {
    SEARCH_JOB_STATUS,
    SEARCH_JOB_STATUS_WAITING_STATES,
} from "../constants";


/**
 * Interval in milliseconds for polling the completion status of a job.
 */
const JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS = 0.5;

/**
 * Enum of the `search_jobs` table's column names.
 *
 * @enum {string}
 */
const SEARCH_JOBS_TABLE_COLUMN_NAMES = Object.freeze({
    ID: "id",
    STATUS: "status",
    JOB_CONFIG: "job_config",
});

/**
 * Class for submitting and monitoring search jobs in the database.
 */
class SearchJobsDbManager {
    #sqlDbConnPool;

    #searchJobsTableName;

    /**
     * @param {import("mysql2/promise").Pool} sqlDbConnPool
     * @param {object} tableNames
     * @param {string} tableNames.searchJobsTableName
     */
    constructor (sqlDbConnPool, {searchJobsTableName}) {
        this.#sqlDbConnPool = sqlDbConnPool;
        this.#searchJobsTableName = searchJobsTableName;
    }

    /**
     * Submits a search job to the database.
     *
     * @param {object} searchConfig The arguments for the query.
     * @return {Promise<number>} The job's ID.
     * @throws {Error} on error.
     */
    async submitSearchJob (searchConfig) {
        const [queryInsertResults] = await this.#sqlDbConnPool.query(
            `INSERT INTO ${this.#searchJobsTableName}
                 (${SEARCH_JOBS_TABLE_COLUMN_NAMES.SEARCH_CONFIG})
             VALUES (?)`,
            [Buffer.from(msgpack.encode(searchConfig))],
        );

        return queryInsertResults.insertId;
    }

    /**
     * Submits an aggregation job to the database.
     *
     * @param {object} searchConfig The arguments for the query.
     * @param {number} timeRangeBucketSizeMillis
     * @return {Promise<number>} The aggregation job's ID.
     * @throws {Error} on error.
     */
    async submitAggregationJob (searchConfig, timeRangeBucketSizeMillis) {
        const searchAggregationConfig = {
            ...searchConfig,
            aggregation_config: {
                count_by_time_bucket_size: timeRangeBucketSizeMillis,
            },
        };

        return await this.submitSearchJob(searchAggregationConfig);
    }

    /**
     * Submits a query cancellation request to the database.
     *
     * @param {number} jobId ID of the job to cancel.
     * @return {Promise<void>}
     * @throws {Error} on error.
     */
    async submitQueryCancellation (jobId) {
        await this.#sqlDbConnPool.query(
            `UPDATE ${this.#searchJobsTableName}
             SET ${SEARCH_JOBS_TABLE_COLUMN_NAMES.STATUS} = ${SEARCH_JOB_STATUS.CANCELLING}
             WHERE ${SEARCH_JOBS_TABLE_COLUMN_NAMES.ID} = ?
             AND ${SEARCH_JOBS_TABLE_COLUMN_NAMES.STATUS}
             IN (${SEARCH_JOB_STATUS.PENDING}, ${SEARCH_JOB_STATUS.RUNNING})`,
            jobId,
        );
    }

    /**
     * Waits for the job to complete.
     *
     * @param {number} jobId
     * @return {Promise<void>}
     * @throws {Error} on MySQL error, if the job wasn't found in the database, if the job was
     * cancelled, or if the job completed in an unexpected state.
     */
    async awaitJobCompletion (jobId) {
        while (true) {
            let rows;
            try {
                const [queryRows] = await this.#sqlDbConnPool.query(
                    `
                    SELECT ${SEARCH_JOBS_TABLE_COLUMN_NAMES.STATUS}
                    FROM ${this.#searchJobsTableName}
                    WHERE ${SEARCH_JOBS_TABLE_COLUMN_NAMES.ID} = ?
                    `,
                    jobId,
                );

                rows = queryRows;
            } catch (e) {
                throw new Error(`Failed to query status for job ${jobId} - ${e}`);
            }
            if (0 === rows.length) {
                throw new Error(`Job ${jobId} not found in database.`);
            }
            const status = rows[0][SEARCH_JOBS_TABLE_COLUMN_NAMES.STATUS];

            if (false === SEARCH_JOB_STATUS_WAITING_STATES.includes(status)) {
                if (SEARCH_JOB_STATUS.CANCELLED === status) {
                    throw new Error(`Job ${jobId} was cancelled.`);
                } else if (SEARCH_JOB_STATUS.SUCCEEDED !== status) {
                    throw new Error(`Job ${jobId} exited with unexpected status=${status}: ` +
                        `${Object.keys(SEARCH_JOB_STATUS)[status]}.`);
                }
                break;
            }

            await sleep(JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS);
        }
    }
}

export default SearchJobsDbManager;
