import {Nullable} from "src/typings/common";

import {
    CLP_ARCHIVES_TABLE_COLUMN_NAMES,
    CLP_FILES_TABLE_COLUMN_NAMES,
} from "../sqlConfig";


/**
 * Builds the query string to query stats.
 *
 * @param datasetNames
 * @return
 */
const buildMultiDatasetDetailsSql = (datasetNames: string[]): string => {
    const archiveQueries = datasetNames.map((name) => `
    SELECT
      MIN(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.BEGIN_TIMESTAMP}) AS begin_timestamp,
      MAX(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP}) AS end_timestamp
    FROM clp_${name}_archives
  `);

    const fileQueries = datasetNames.map((name) => `
    SELECT
      COUNT(DISTINCT ${CLP_FILES_TABLE_COLUMN_NAMES.ORIG_FILE_ID}) AS num_files,
      CAST(
        COALESCE(SUM(${CLP_FILES_TABLE_COLUMN_NAMES.NUM_MESSAGES}), 0) AS INTEGER
      ) AS num_messages
    FROM clp_${name}_files
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

interface DetailsItem {
    begin_timestamp: Nullable<number>;
    end_timestamp: Nullable<number>;
    num_files: Nullable<number>;
    num_messages: Nullable<number>;
}

const DETAILS_DEFAULT: DetailsItem = {
    begin_timestamp: null,
    end_timestamp: null,
    num_files: 0,
    num_messages: 0,
};

export type {DetailsItem};
export {
    buildMultiDatasetDetailsSql, DETAILS_DEFAULT,
};
