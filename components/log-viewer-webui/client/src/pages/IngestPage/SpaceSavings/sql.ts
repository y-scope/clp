import {SQL_CONFIG} from "../sqlConfig";


/**
 * Enum of the column names for the `clp_archives` table.
 *
 * @enum {string}
 */
const CLP_ARCHIVES_TABLE_COLUMN_NAMES = Object.freeze({
    UNCOMPRESSED_SIZE: "uncompressed_size",
    SIZE: "size",
});

/**
 * Builds the query string to query stats.
 *
 * @return
 */
const getQueryStatsSql = () => `
SELECT
    SUM(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE}) AS total_uncompressed_size,
    SUM(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.SIZE})              AS total_compressed_size
FROM ${SQL_CONFIG.SqlDbClpArchivesTableName}
`;

interface SpaceSavingsItem {
    total_uncompressed_size: number;
    total_compressed_size: number;
}

type SpaceSavingsResp = SpaceSavingsItem[];

export type {
    SpaceSavingsItem, SpaceSavingsResp,
};
export {getQueryStatsSql as getSpaceSavingsSql};
