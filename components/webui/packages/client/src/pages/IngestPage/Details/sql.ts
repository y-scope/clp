import {SqlTableSuffix} from "@webui/common/config";
import {Nullable} from "@webui/common/utility-types";

import {querySql} from "../../../api/sql";
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
 * @return
 */
const getDetailsSql = () => `
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
    FROM ${settings.SqlDbClpArchivesTableName}
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
    FROM ${settings.SqlDbClpFilesTableName}
) b;
`;

/**
 * Builds the query string for details stats when using CLP-S storage engine
 * (i.e. multiple datasets).
 *
 * @param datasetNames
 * @return
 */
const buildMultiDatasetDetailsSql = (datasetNames: string[]): string => {
    const archiveQueries = datasetNames.map((name) => `
    SELECT
      MIN(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.BEGIN_TIMESTAMP}) AS begin_timestamp,
      MAX(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP}) AS end_timestamp
    FROM ${settings.SqlDbClpTablePrefix}${name}_${SqlTableSuffix.ARCHIVES}
  `);

    const fileQueries = datasetNames.map((name) => `
    SELECT
      COUNT(DISTINCT ${CLP_FILES_TABLE_COLUMN_NAMES.ORIG_FILE_ID}) AS num_files,
      CAST(
        COALESCE(SUM(${CLP_FILES_TABLE_COLUMN_NAMES.NUM_MESSAGES}), 0) AS UNSIGNED
      ) AS num_messages
    FROM ${settings.SqlDbClpTablePrefix}${name}_${SqlTableSuffix.FILES}
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
    const sql = getDetailsSql();
    return executeDetailsQuery(sql);
};

/**
 * Fetches details statistics when using CLP-S storage engine.
 *
 * @param datasetNames
 * @return
 */
const fetchClpsDetails = async (
    datasetNames: string[]
): Promise<DetailsItem> => {
    if (0 === datasetNames.length) {
        return DETAILS_DEFAULT;
    }
    const sql = buildMultiDatasetDetailsSql(datasetNames);
    return executeDetailsQuery(sql);
};

export type {DetailsItem};
export {
    DETAILS_DEFAULT,
    fetchClpDetails,
    fetchClpsDetails,
};
