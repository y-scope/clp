import {Nullable} from "src/typings/common";

import {settings} from "../../../settings";
import {
    CLP_ARCHIVES_TABLE_COLUMN_NAMES,
    CLP_FILES_TABLE_COLUMN_NAMES,
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
    FROM ${settings.SqlDbClpArchivesTableName}
) a,
(
    SELECT
        COUNT(DISTINCT ${CLP_FILES_TABLE_COLUMN_NAMES.ORIG_FILE_ID})   AS num_files,
        CAST(
            COALESCE(
                SUM(${CLP_FILES_TABLE_COLUMN_NAMES.NUM_MESSAGES}),
                0
            ) AS INTEGER
        ) AS num_messages
    FROM ${settings.SqlDbClpFilesTableName}
) b;
`;

interface DetailsItem {
    begin_timestamp: Nullable<number>;
    end_timestamp: Nullable<number>;
    num_files: Nullable<number>;
    num_messages: Nullable<number>;
}

type DetailsResp = DetailsItem[];

export type {
    DetailsItem,
    DetailsResp,
};
export {getDetailsSql};
