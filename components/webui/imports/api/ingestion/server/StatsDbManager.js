/**
 * Enum of the column names for the `clp_archives` table.
 *
 * @enum {string}
 */
const CLP_ARCHIVES_TABLE_COLUMN_NAMES = Object.freeze({
    BEGIN_TIMESTAMP: "begin_timestamp",
    END_TIMESTAMP: "end_timestamp",
    UNCOMPRESSED_SIZE: "uncompressed_size",
    SIZE: "size",
});

/**
 * Enum of the column names for the `clp_files` table.
 *
 * @enum {string}
 */
const CLP_FILES_TABLE_COLUMN_NAMES = Object.freeze({
    ORIG_FILE_ID: "orig_file_id",
    NUM_MESSAGES: "num_messages",
});

/**
 * Class for retrieving compression stats from the database.
 */
class StatsDbManager {
    #sqlDbConnPool;

    #clpArchivesTableName;

    #clpFilesTableName;

    /**
     * @param {import("mysql2/promise").Pool} sqlDbConnPool
     * @param {object} tableNames
     * @param {string} tableNames.clpArchivesTableName
     * @param {string} tableNames.clpFilesTableName
     */
    constructor (sqlDbConnPool, {
        clpArchivesTableName,
        clpFilesTableName,
    }) {
        this.#sqlDbConnPool = sqlDbConnPool;

        this.#clpArchivesTableName = clpArchivesTableName;
        this.#clpFilesTableName = clpFilesTableName;
    }

    /**
     * Queries compression stats.
     *
     * @return {Promise<CompressionStats>}
     * @throws {Error} on error.
     */
    async getCompressionStats () {
        /* eslint-disable @stylistic/js/max-len */
        const [queryStats] = await this.#sqlDbConnPool.query(`
            SELECT
                a.begin_timestamp         AS begin_timestamp,
                a.end_timestamp           AS end_timestamp,
                a.total_uncompressed_size AS total_uncompressed_size,
                a.total_compressed_size   AS total_compressed_size,
                b.num_files               AS num_files,
                b.num_messages            AS num_messages
            FROM
            (
                SELECT
                    MIN(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.BEGIN_TIMESTAMP})   AS begin_timestamp,
                    MAX(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP})     AS end_timestamp,
                    SUM(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE}) AS total_uncompressed_size,
                    SUM(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.SIZE})              AS total_compressed_size
                FROM ${this.#clpArchivesTableName}
            ) a,
            (
                SELECT
                    NULLIF(COUNT(DISTINCT ${CLP_FILES_TABLE_COLUMN_NAMES.ORIG_FILE_ID}), 0) AS num_files,
                    SUM(${CLP_FILES_TABLE_COLUMN_NAMES.NUM_MESSAGES})                       AS num_messages
                FROM ${this.#clpFilesTableName}
            ) b;
        `);
        /* eslint-enable @stylistic/js/max-len */

        return queryStats[0];
    }
}

export default StatsDbManager;
