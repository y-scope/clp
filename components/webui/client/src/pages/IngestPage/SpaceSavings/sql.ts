import {settings} from "../../../settings";
import {CLP_ARCHIVES_TABLE_COLUMN_NAMES} from "../sqlConfig";


/**
 * Builds the query string to query stats.
 *
 * @return
 */
const getSpaceSavingsSql = () => `
SELECT
    CAST(
        COALESCE(
            SUM(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE}),
            0
        ) AS UNSIGNED
    ) AS total_uncompressed_size,
    CAST(
        COALESCE(
            SUM(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.SIZE}),
            0
        ) AS UNSIGNED
    ) AS total_compressed_size
FROM ${settings.SqlDbClpArchivesTableName}
`;

interface SpaceSavingsItem {
    total_uncompressed_size: number;
    total_compressed_size: number;
}

type SpaceSavingsResp = SpaceSavingsItem[];

export type {
    SpaceSavingsItem,
    SpaceSavingsResp,
};
export {getSpaceSavingsSql};
