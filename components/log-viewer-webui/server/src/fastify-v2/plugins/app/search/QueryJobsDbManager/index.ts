import {setTimeout} from "node:timers/promises";

import type {MySQLPromisePool} from "@fastify/mysql";
import {encode} from "@msgpack/msgpack";
import {FastifyInstance} from "fastify";
import fp from "fastify-plugin";
import {ResultSetHeader} from "mysql2";

import settings from "../../../../../../settings.json" with {type: "json"};
import {
    QUERY_JOB_STATUS,
    QUERY_JOB_STATUS_WAITING_STATES,
    QUERY_JOB_TYPE,
    QUERY_JOBS_TABLE_COLUMN_NAMES,
    QueryJob,
} from "../../../../../typings/query.js";
import {JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS} from "./typings.js";


/**
 * Class for submitting and monitoring query jobs in the database.
 */
class QueryJobsDbManager {
    #sqlDbConnPool: MySQLPromisePool;

    private constructor (sqlDbConnPool: MySQLPromisePool) {
        this.#sqlDbConnPool = sqlDbConnPool;
    }

    /**
     * Creates a new QueryJobsDbManager.
     *
     * @param fastify
     * @return
     */
    static create (fastify: FastifyInstance): QueryJobsDbManager {
        const sqlDbConnPool = fastify.mysql;
        return new QueryJobsDbManager(sqlDbConnPool);
    }

    /**
     * Submits a search job to the database.
     *
     * @param searchConfig The arguments for the query.
     * @return The job's ID.
     * @throws {Error} on error.
     */
    async submitSearchJob (searchConfig: object): Promise<number> {
        const [queryInsertResults] = await this.#sqlDbConnPool.query<ResultSetHeader>(
            `
            INSERT INTO ${settings.SqlDbQueryJobsTableName} (
               ${QUERY_JOBS_TABLE_COLUMN_NAMES.JOB_CONFIG},
               ${QUERY_JOBS_TABLE_COLUMN_NAMES.TYPE}
            )
            VALUES (?, ?)
            `,
            [
                Buffer.from(encode(searchConfig)),
                QUERY_JOB_TYPE.SEARCH_OR_AGGREGATION,
            ]
        );

        return queryInsertResults.insertId;
    }

    /**
     * Submits an aggregation job to the database.
     *
     * @param searchConfig The arguments for the query.
     * @param timeRangeBucketSizeMillis
     * @return The aggregation job's ID.
     * @throws {Error} on error.
     */
    async submitAggregationJob (
        searchConfig: object,
        timeRangeBucketSizeMillis: number
    ): Promise<number> {
        const searchAggregationConfig = {
            ...searchConfig,
            aggregation_config: {
                count_by_time_bucket_size: timeRangeBucketSizeMillis,
            },
        };

        return this.submitSearchJob(searchAggregationConfig);
    }

    /**
     * Submits a query cancellation request to the database.
     *
     * @param jobId ID of the job to cancel.
     * @return
     * @throws {Error} on error.
     */
    async submitQueryCancellation (jobId: number): Promise<void> {
        await this.#sqlDbConnPool.query(
            `
            UPDATE ${settings.SqlDbQueryJobsTableName}
            SET ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS} = ${QUERY_JOB_STATUS.CANCELLING}
            WHERE ${QUERY_JOBS_TABLE_COLUMN_NAMES.ID} = ?
            AND ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS}
            IN (${QUERY_JOB_STATUS.PENDING}, ${QUERY_JOB_STATUS.RUNNING})
            `,
            jobId,
        );
    }

    /**
     * Waits for the job to complete.
     *
     * @param jobId
     * @return
     * @throws {Error} on MySQL error, if the job wasn't found in the database, if the job was
     * cancelled, or if the job completed in an unexpected state.
     */
    async awaitJobCompletion (jobId: number): Promise<void> {
        // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
        while (true) {
            let queryJob: QueryJob | undefined;
            try {
                const [queryRows] = await this.#sqlDbConnPool.query<QueryJob[]>(
                    `
                    SELECT ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS}
                    FROM ${settings.SqlDbQueryJobsTableName}
                    WHERE ${QUERY_JOBS_TABLE_COLUMN_NAMES.ID} = ?
                    `,
                    jobId
                );

                [queryJob] = queryRows;
            } catch (e: unknown) {
                throw new Error(`Failed to query status for job ${jobId}`, {cause: e});
            }
            if ("undefined" === typeof queryJob) {
                throw new Error(`Job ${jobId} not found in the database.`);
            }

            const status = queryJob[QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS];

            if (false === QUERY_JOB_STATUS_WAITING_STATES.has(status)) {
                if (QUERY_JOB_STATUS.CANCELLED === status) {
                    throw new Error(`Job ${jobId} was cancelled.`);
                } else if (QUERY_JOB_STATUS.SUCCEEDED !== status) {
                    throw new Error(
                        `Job ${jobId} exited with unexpected status=${status}: ` +
                        `${Object.keys(QUERY_JOB_STATUS)[status]}.`
                    );
                }
                break;
            }

            await setTimeout(JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS);
        }
    }
}

declare module "fastify" {
    export interface FastifyInstance {
        QueryJobsDbManager: QueryJobsDbManager;
    }
}

export default fp(
    (fastify) => {
        fastify.decorate("QueryJobsDbManager", QueryJobsDbManager.create(fastify));
    },
    {
        name: "QueryJobsDbManager",
    }
);
