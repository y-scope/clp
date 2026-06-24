import {setTimeout} from "node:timers/promises";

import type {MySQLPromisePool} from "@fastify/mysql";
import {encode} from "@msgpack/msgpack";
import {QUERY_JOB_TYPE} from "@webui/common/query";
import {FastifyInstance} from "fastify";
import fp from "fastify-plugin";
import {ResultSetHeader} from "mysql2";

import settings from "../../../../settings.json" with {type: "json"};
import {
    QUERY_JOB_STATUS,
    QUERY_JOB_STATUS_WAITING_STATES,
    QUERY_JOBS_TABLE_COLUMN_NAMES,
    QueryJob,
} from "../../../typings/query.js";
import {JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS} from "./typings.js";


/**
 * Class for managing jobs in the CLP package query scheduler database.
 */
class QueryJobDbManager {
    readonly #sqlPool: MySQLPromisePool;

    readonly #tableName: string;

    private constructor (sqlPool: MySQLPromisePool, tableName: string) {
        this.#sqlPool = sqlPool;
        this.#tableName = tableName;
    }

    /**
     * Creates a new QueryDbJobManager.
     *
     * @param fastify
     * @return
     */
    static create (fastify: FastifyInstance): QueryJobDbManager {
        return new QueryJobDbManager(fastify.mysql, settings.SqlDbQueryJobsTableName);
    }

    /**
     * Submits a job to the database.
     *
     * @param jobConfig
     * @param jobType
     * @return The job's ID.
     * @throws {Error} on error.
     */
    async submitJob (jobConfig: object, jobType: QUERY_JOB_TYPE): Promise<number> {
        const [result] = await this.#sqlPool.query<ResultSetHeader>(
            `
            INSERT INTO ${this.#tableName} (
               ${QUERY_JOBS_TABLE_COLUMN_NAMES.JOB_CONFIG},
               ${QUERY_JOBS_TABLE_COLUMN_NAMES.TYPE}
            )
            VALUES (?, ?)
            `,
            [
                Buffer.from(encode(jobConfig)),
                jobType,
            ]
        );

        return result.insertId;
    }

    /**
     * Submits a job cancellation request to the database.
     *
     * @param jobId ID of the job to cancel.
     * @throws {Error} on error.
     */
    async cancelJob (jobId: number): Promise<void> {
        await this.#sqlPool.query(
            `UPDATE ${this.#tableName}
             SET ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS} = ${QUERY_JOB_STATUS.CANCELLING}
             WHERE ${QUERY_JOBS_TABLE_COLUMN_NAMES.ID} = ?
             AND ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS}
             IN (${QUERY_JOB_STATUS.PENDING}, ${QUERY_JOB_STATUS.RUNNING})`,
            jobId
        );
    }

    /**
     * Waits for the job with the given ID to finish.
     *
     * @param jobId
     * @throws {Error} on MySQL error, if the job wasn't found in the database, if the job was
     * cancelled, or if the job completed in an unexpected state.
     */
    async awaitJobCompletion (jobId: number): Promise<void> {
        // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
        while (true) {
            let queryJob: QueryJob | undefined;
            try {
                const [queryRows] = await this.#sqlPool.query<QueryJob[]>(
                    `SELECT ${QUERY_JOBS_TABLE_COLUMN_NAMES.STATUS}
                     FROM ${this.#tableName}
                     WHERE ${QUERY_JOBS_TABLE_COLUMN_NAMES.ID} = ?`,
                    jobId
                );

                [queryJob] = queryRows;
            } catch (e: unknown) {
                throw new Error(`Failed to query status for job ${jobId}`, {cause: e});
            }

            if ("undefined" === typeof queryJob) {
                throw new Error(`Job ${jobId} not found in database.`);
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

    /**
     * Submits a job and waits for it to complete.
     *
     * @param jobConfig
     * @param jobType
     * @return The job's ID.
     * @throws {Error} on error.
     */
    async submitAndWaitForJob (jobConfig: object, jobType: QUERY_JOB_TYPE): Promise<number> {
        const jobId = await this.submitJob(jobConfig, jobType);
        await this.awaitJobCompletion(jobId);

        return jobId;
    }
}

declare module "fastify" {
    interface FastifyInstance {
        QueryJobDbManager: QueryJobDbManager;
    }
}

export default fp(
    (fastify) => {
        fastify.decorate("QueryJobDbManager", QueryJobDbManager.create(fastify));
    },
    {
        name: "QueryJobDbManager",
    }
);
