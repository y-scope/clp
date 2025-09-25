import {SqlTableSuffix} from "../../../../../config/sql-table-suffix";
import {settings} from "../../../../../settings";
import {CLP_ARCHIVES_TABLE_COLUMN_NAMES} from "../../../../IngestPage/sqlConfig";


/**
 * Builds a SQL query string to retrieve the minimum and maximum timestamps from the CLP archives
 * table.
 *
 * @return
 */
const buildClpTimeRangeSql = () => `SELECT
    MIN(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.BEGIN_TIMESTAMP})   AS begin_timestamp,
    MAX(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP})     AS end_timestamp
FROM ${settings.SqlDbClpArchivesTableName}
`;

/**
 * Builds a SQL query string to retrieve the minimum and maximum timestamps for a specific CLP-s
 * dataset's archives.
 *
 * @param datasetName
 * @return
 */
const buildClpsTimeRangeSql = (datasetName: string): string => {
    return `SELECT
  MIN(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.BEGIN_TIMESTAMP}) AS begin_timestamp,
  MAX(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP}) AS end_timestamp
FROM ${settings.SqlDbClpTablePrefix}${datasetName}_${SqlTableSuffix.ARCHIVES}`;
};

export {
    buildClpsTimeRangeSql, buildClpTimeRangeSql,
};
