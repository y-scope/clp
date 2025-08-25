import {Nullable} from "src/typings/common";

import {querySql} from "../../../api/sql";
import {SqlTableSuffix} from "../../../config/sql-table-suffix";
import {settings} from "../../../settings";
import {
    CLP_ARCHIVES_TABLE_COLUMN_NAMES,
    CLP_FILES_TABLE_COLUMN_NAMES,
} from "../sqlConfig";


/**
 * Result from SQL details query.
 */
interface DetailsItem {
    begin_timestamp: Nullable<number>;
    end_timestamp: Nullable<number>;
    num_files: Nullable<number>;
    num_messages: Nullable<number>;
}

/**
 * Default values for details when no data is available.
 */
const DETAILS_DEFAULT: DetailsItem = {
    begin_timestamp: null,
    end_timestamp: null,
    num_files: 0,
    num_messages: 0,
};

/**
 * Builds the query string for details stats when using CLP storage engine (i.e. no datasets).
 *
 * @param archiveEndTsLowerBound Lower bound for archive end timestamp in
 * milliseconds. When null, no retention filtering is applied.
 * @return
 */
const getDetailsSql = (archiveEndTsLowerBound: number | null) => {
    const archiveRetentionFilter = null !== archiveEndTsLowerBound ?
        ` WHERE ${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP} >= ` +
          `${archiveEndTsLowerBound} OR ${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP} = 0` :
        "";

    const fileRetentionFilter = null !== archiveEndTsLowerBound ?
        ` WHERE ${CLP_FILES_TABLE_COLUMN_NAMES.ARCHIVE_ID} IN (` +
          `SELECT ${CLP_ARCHIVES_TABLE_COLUMN_NAMES.ID} FROM ` +
          `${settings.SqlDbClpArchivesTableName}${archiveRetentionFilter})` :
        "";

    return `
SELECT
    a.begin_timestamp         AS begin_timestamp,
    a.end_timestamp           AS end_timestamp,
    b.num_files               AS num_files,
    b.num_messages            AS num_messages
FROM
(
    SELECT
        MIN(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.BEGIN_TIMESTAMP})   AS begin_timestamp,
        MAX(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP})     AS end_timestamp
    FROM ${settings.SqlDbClpArchivesTableName}${archiveRetentionFilter}
) a,
(
    SELECT
        COUNT(DISTINCT ${CLP_FILES_TABLE_COLUMN_NAMES.ORIG_FILE_ID})   AS num_files,
        CAST(
            COALESCE(
                SUM(${CLP_FILES_TABLE_COLUMN_NAMES.NUM_MESSAGES}),
                0
            ) AS UNSIGNED
        ) AS num_messages
    FROM ${settings.SqlDbClpFilesTableName}${fileRetentionFilter}
) b;
`;
};

/**
 * Builds the query string for details stats when using CLP-S storage engine
 * (i.e. multiple datasets).
 *
 * @param datasetNames
 * @param archiveEndTsLowerBound Lower bound for archive end timestamp in milliseconds.
 * When null, no retention filtering is applied.
 * @return
 */
const buildMultiDatasetDetailsSql = (
    datasetNames: string[],
    archiveEndTsLowerBound: number | null
): string => {
    const archiveRetentionFilter = null !== archiveEndTsLowerBound ?
        ` WHERE ${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP} >= ` +
          `${archiveEndTsLowerBound} OR ${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP} = 0` :
        "";

    const archiveQueries = datasetNames.map((name) => `
    SELECT
      MIN(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.BEGIN_TIMESTAMP}) AS begin_timestamp,
      MAX(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP}) AS end_timestamp
    FROM ${settings.SqlDbClpTablePrefix}${name}_${SqlTableSuffix.ARCHIVES}${archiveRetentionFilter}
  `);

    const getFileRetentionFilter = (name: string) => (null !== archiveEndTsLowerBound ?
        ` WHERE ${CLP_FILES_TABLE_COLUMN_NAMES.ARCHIVE_ID} IN (` +
          `SELECT ${CLP_ARCHIVES_TABLE_COLUMN_NAMES.ID} FROM ` +
          `${settings.SqlDbClpTablePrefix}${name}_${SqlTableSuffix.ARCHIVES}${archiveRetentionFilter})` :
        "");

    const fileQueries = datasetNames.map((name) => `
    SELECT
      COUNT(DISTINCT ${CLP_FILES_TABLE_COLUMN_NAMES.ORIG_FILE_ID}) AS num_files,
      CAST(
        COALESCE(SUM(${CLP_FILES_TABLE_COLUMN_NAMES.NUM_MESSAGES}), 0) AS UNSIGNED
      ) AS num_messages
    FROM ${settings.SqlDbClpTablePrefix}${name}_${SqlTableSuffix.FILES}${
        getFileRetentionFilter(name)
    }
  `);

    return `
    SELECT
      a.begin_timestamp,
      a.end_timestamp,
      b.num_files,
      b.num_messages
    FROM
    (
      SELECT
        MIN(begin_timestamp) AS begin_timestamp,
        MAX(end_timestamp)   AS end_timestamp
      FROM (
        ${archiveQueries.join("\nUNION ALL\n")}
      ) AS archives_combined
    ) a,
    (
      SELECT
        SUM(num_files)    AS num_files,
        SUM(num_messages) AS num_messages
      FROM (
        ${fileQueries.join("\nUNION ALL\n")}
      ) AS files_combined
    ) b;
  `;
};

/**
 * Calculates the archive end timestamp lower bound based on retention period.
 *
 * @return The lower bound timestamp in milliseconds, or null if no retention period is set
 */
const calculateArchiveEndTsLowerBound = (): number | null => {
    const MINUTES_TO_MILLISECONDS = 60 * 1000;
    return null !== settings.ArchiveRetentionPeriod ?
        Date.now() - (settings.ArchiveRetentionPeriod * MINUTES_TO_MILLISECONDS) :
        null;
};

/**
 * Executes details SQL query and extracts details result.
 *
 * @param sql
 * @return
 * @throws {Error} if query result does not contain data
 */
const executeDetailsQuery = async (sql: string): Promise<DetailsItem> => {
    const resp = await querySql<DetailsItem[]>(sql);
    const [detailsResult] = resp.data;
    if ("undefined" === typeof detailsResult) {
        throw new Error("Details result does not contain data.");
    }

    return detailsResult;
};

/**
 * Fetches details statistics when using CLP storage engine.
 *
 * @return
 */
const fetchClpDetails = async (): Promise<DetailsItem> => {
    const archiveEndTsLowerBound = calculateArchiveEndTsLowerBound();
    const sql = getDetailsSql(archiveEndTsLowerBound);
    return executeDetailsQuery(sql);
};

/**
 * Fetches details statistics when using CLP-S storage engine.
 *
 * @param datasetNames
 * @return
 */
const fetchClpsDetails = async (datasetNames: string[]): Promise<DetailsItem> => {
    if (0 === datasetNames.length) {
        return DETAILS_DEFAULT;
    }
    const archiveEndTsLowerBound = calculateArchiveEndTsLowerBound();
    const sql = buildMultiDatasetDetailsSql(datasetNames, archiveEndTsLowerBound);
    return executeDetailsQuery(sql);
};

export {
    calculateArchiveEndTsLowerBound,
    DETAILS_DEFAULT,
    fetchClpDetails,
    fetchClpsDetails,
};
