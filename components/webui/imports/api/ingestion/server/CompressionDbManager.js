import {COMPRESSION_JOBS_TABLE_COLUMN_NAMES} from "../constants";


/**
 * Class for retrieving compression jobs from the database.
 */
class CompressionDbManager {
    #sqlDbConnPool;

    #compressionJobsTableName;

    /**
     * @param {import("mysql2/promise").Pool} sqlDbConnPool
     * @param {object} tableNames
     * @param {string} tableNames.compressionJobsTableName
     */
    constructor (sqlDbConnPool, {compressionJobsTableName}) {
        this.#sqlDbConnPool = sqlDbConnPool;
        this.#compressionJobsTableName = compressionJobsTableName;
    }

    /**
     * Retrieves compression jobs that are updated on or after a specific time.
     *
     * @param {number} lastUpdateTimestampSeconds
     * @return {Promise<object[]>} Job objects with fields with the names in
     * `COMPRESSION_JOBS_TABLE_COLUMN_NAMES`
     */
    async getCompressionJobs (lastUpdateTimestampSeconds) {
        const queryString = `
            SELECT
                UNIX_TIMESTAMP() as retrieval_time,
                id as _id,
                ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS},
                ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS_MSG},
                ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.START_TIME},
                ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UPDATE_TIME},
                ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.DURATION},
                ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE},
                ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.COMPRESSED_SIZE}
            FROM ${this.#compressionJobsTableName}
            WHERE ${COMPRESSION_JOBS_TABLE_COLUMN_NAMES.UPDATE_TIME} >= 
                FROM_UNIXTIME(${lastUpdateTimestampSeconds}) - 1
            ORDER BY _id DESC;`;

        const [results] = await this.#sqlDbConnPool.query(queryString);

        return results;
    }
}

export default CompressionDbManager;
