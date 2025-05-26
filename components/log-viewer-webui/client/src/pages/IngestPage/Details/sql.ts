import {
    CLP_ARCHIVES_TABLE_COLUMN_NAMES,
    CLP_FILES_TABLE_COLUMN_NAMES,
    SQL_CONFIG,
} from "../sqlConfig";


/**
 * Builds the query string to query stats.
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
    FROM ${SQL_CONFIG.SqlDbClpArchivesTableName}
) a,
(
    SELECT
        NULLIF(COUNT(DISTINCT ${CLP_FILES_TABLE_COLUMN_NAMES.ORIG_FILE_ID}), 0) AS num_files,
        SUM(${CLP_FILES_TABLE_COLUMN_NAMES.NUM_MESSAGES})                       AS num_messages
    FROM ${SQL_CONFIG.SqlDbClpFilesTableName}
) b;
`;

interface DetailsItem {
    begin_timestamp: number;
    end_timestamp: number;
    num_files: number;
    num_messages: number;
}

type DetailsResp = DetailsItem[];

export type {
    DetailsItem,
    DetailsResp,
};
export {getDetailsSql};
