import {encode} from "@msgpack/msgpack";

import { ResultSetHeader, RowDataPacket } from 'mysql2';

import {
    QUERY_JOB_STATUS,
    QUERY_JOB_STATUS_WAITING_STATES,
    QUERY_JOB_TYPE,
} from "./constants.js";

import { FastifyInstance } from 'fastify'
import fp from 'fastify-plugin'

import settings from "../../../../settings.json" with {type: "json"};

import { sleep } from "../../../utils/misc.js";

import { Collection, Document } from "mongodb";

/**
 * Interval in milliseconds for polling the completion status of a job.
 */
const JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS = 0.5;

/**
 * Enum of the `query_jobs` table's column names.
 *
 * @enum {string}
 */
const QUERY_JOBS_TABLE_COLUMN_NAMES = Object.freeze({
    ID: "id",
    STATUS: "status",
    TYPE: "type",
    JOB_CONFIG: "job_config",
});


declare module 'fastify' {
  export interface FastifyInstance {
    QueryJobsDbManager: ReturnType<typeof createQueryJobsDbManager>;
  }
}



/**
 * Class for submitting and monitoring query jobs in the database.
 */
function createQueryJobsDbManager (fastify: FastifyInstance) {
    const sqlDbConnPool = fastify.MYSQL2

  return {
     /**
     * Submits a search job to the database.
     *
     * @param {object} searchConfig The arguments for the query.
     * @return {Promise<number>} The job's ID.
     * @throws {Error} on error.
     */
     async submitSearchJob (searchConfig: object) {
        const [queryInsertResults] = await sqlDbConnPool.query<ResultSetHeader>(
            `INSERT INTO ${settings.SqlDbQueryJobsTableName}
                 (${QUERY_JOBS_TABLE_COLUMN_NAMES.JOB_CONFIG},
                  ${QUERY_JOBS_TABLE_COLUMN_NAMES.TYPE})
             VALUES (?, ?)`,
            [Buffer.from(encode(searchConfig)),
                QUERY_JOB_TYPE.SEARCH_OR_AGGREGATION],
        );

        return queryInsertResults.insertId;
    },



    /**
     * Submits an aggregation job to the database.
     *
     * @param {object} searchConfig The arguments for the query.
     * @param {number} timeRangeBucketSizeMillis
     * @return {Promise<number>} The aggregation job's ID.
     * @throws {Error} on error.
     */
    async submitAggregationJob (searchConfig: object, timeRangeBucketSizeMillis: number) {
        const searchAggregationConfig = {
            ...searchConfig,
            aggregation_config: {
                count_by_time_bucket_size: timeRangeBucketSizeMillis,
            },
        };

        return await this.submitSearchJob(searchAggregationConfig);
    },

    /**
     * Submits a query cancellation request to the database.
     *
     * @param {number} jobId ID of the job to cancel.
     * @return {Promise<void>}
     * @throws {Error} on error.
     */
    async submitQueryCancellation (jobId: number) {
        await sqlDbConnPool.query(
            `UPDATE ${settings.SqlDbQueryJobsTableName  }
                SET ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS} = ${QUERY_JOB_STATUS.CANCELLING}
                WHERE ${QUERY_JOBS_TABLE_COLUMN_NAMES.ID} = ?
                AND ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS}
                IN (${QUERY_JOB_STATUS.PENDING}, ${QUERY_JOB_STATUS.RUNNING})`,
            jobId,
        );
    },

    /**
     * Waits for the job to complete.
     *
     * @param {number} jobId
     * @return {Promise<void>}
     * @throws {Error} on MySQL error, if the job wasn't found in the database, if the job was
     * cancelled, or if the job completed in an unexpected state.
     */
    async awaitJobCompletion (jobId: number) {
        while (true) {
            let rows;
            try {
                const [queryRows] = await sqlDbConnPool.query<RowDataPacket[]>(
                    `
                    SELECT ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS}
                    FROM ${settings.SqlDbQueryJobsTableName}
                    WHERE ${QUERY_JOBS_TABLE_COLUMN_NAMES.ID} = ?
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
            const status = rows[0]![QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS];

            if (false === QUERY_JOB_STATUS_WAITING_STATES.includes(status)) {
                if (QUERY_JOB_STATUS.CANCELLED === status) {
                    throw new Error(`Job ${jobId} was cancelled.`);
                } else if (QUERY_JOB_STATUS.SUCCEEDED !== status) {
                    throw new Error(`Job ${jobId} exited with unexpected status=${status}: ` +
                        `${Object.keys(QUERY_JOB_STATUS)[status]}.`);
                }
                break;
            }

            await sleep(JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS);
        }
    }
  }
}

export default fp(
  function (fastify) {
    fastify.decorate('QueryJobsDbManager', createQueryJobsDbManager(fastify))
  },
  {
    name: 'QueryJobsDbManager',
    dependencies: ['SearchJobCollectionsManager', 'SearchResultsMetadataCollection'],
  }
)