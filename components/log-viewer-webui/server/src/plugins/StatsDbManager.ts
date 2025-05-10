/* eslint-disable @stylistic/max-len */
import {
    Pool,
    RowDataPacket,
} from "mysql2/promise";


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
 * Compression statistics.
 */
interface CompressionStats {
    begin_timestamp: string;
    end_timestamp: string;
    total_uncompressed_size: number;
    total_compressed_size: number;
    num_files: number;
    num_messages: number;
}

/**
 * Class for retrieving compression stats from the database.
 */
class StatsDbManager {
    #sqlDbConnPool: Pool;

    #clpArchivesTableName: string;

    #clpFilesTableName: string;

    /**
     * @param sqlDbConnPool
     * @param tableNames
     * @param tableNames.clpArchivesTableName
     * @param tableNames.clpFilesTableName
     */
    constructor (sqlDbConnPool: Pool, {
        clpArchivesTableName,
        clpFilesTableName,
    }: {
        clpArchivesTableName: string;
        clpFilesTableName: string;
    }) {
        this.#sqlDbConnPool = sqlDbConnPool;
        this.#clpArchivesTableName = clpArchivesTableName;
        this.#clpFilesTableName = clpFilesTableName;
    }

    /**
     * Queries compression stats.
     *
     * @return Compression stats object
     * @throws {Error} on error.
     */
    async getCompressionStats () {
        const [queryStats] = await this.#sqlDbConnPool.query<RowDataPacket[]>(`
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

        return queryStats[0] as CompressionStats;
    }
}

export default StatsDbManager;
