import {SqlTableSuffix} from "@webui/common/config";

import {querySql} from "../../../api/sql";
import {settings} from "../../../settings";
import {CLP_ARCHIVES_TABLE_COLUMN_NAMES} from "../sqlConfig";


/**
 * Result from sql space savings query.
 */
interface SpaceSavingsItem {
    total_uncompressed_size: number;
    total_compressed_size: number;
}

/**
 * Default values for space savings when no data is available.
 */
const SPACE_SAVINGS_DEFAULT: SpaceSavingsItem = {
    total_compressed_size: 0,
    total_uncompressed_size: 0,
};


/**
 * Builds the query string for space savings stats when using CLP storage engine (i.e. no datasets).
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

/**
 * Builds the query string for space savings stats when using CLP-S storage
 * engine (i.e. multiple datasets).
 *
 * @param datasetNames
 * @return
 */
const buildMultiDatasetSpaceSavingsSql = (datasetNames: string[]): string => {
    const archiveQueries = datasetNames.map((name) => `
    SELECT
        ${CLP_ARCHIVES_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE},
        ${CLP_ARCHIVES_TABLE_COLUMN_NAMES.SIZE}
    FROM ${settings.SqlDbClpTablePrefix}${name}_${SqlTableSuffix.ARCHIVES}
    `);

    return `
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
    FROM (
        ${archiveQueries.join("\nUNION ALL\n")}
    ) AS archives_combined
    `;
};

/**
 * Executes space savings SQL query and extracts space savings result.
 *
 * @param sql
 * @return
 * @throws {Error} if query result does not contain data
 */
const executeSpaceSavingsQuery = async (sql: string): Promise<SpaceSavingsItem> => {
    const resp = await querySql<SpaceSavingsItem[]>(sql);
    const [spaceSavingsResult] = resp.data;
    if ("undefined" === typeof spaceSavingsResult) {
        throw new Error("Space savings result does not contain data.");
    }

    return spaceSavingsResult;
};

/**
 * Fetches space savings statistics when using CLP storage engine.
 *
 * @return
 */
const fetchClpSpaceSavings = async (): Promise<SpaceSavingsItem> => {
    const sql = getSpaceSavingsSql();
    return executeSpaceSavingsQuery(sql);
};

/**
 * Fetches space savings statistics when using CLP-S storage engine.
 *
 * @param datasetNames
 * @return
 */
const fetchClpsSpaceSavings = async (
    datasetNames: string[]
): Promise<SpaceSavingsItem> => {
    if (0 === datasetNames.length) {
        return SPACE_SAVINGS_DEFAULT;
    }
    const sql = buildMultiDatasetSpaceSavingsSql(datasetNames);
    return executeSpaceSavingsQuery(sql);
};

export type {SpaceSavingsItem};
export {
    fetchClpSpaceSavings,
    fetchClpsSpaceSavings,
    SPACE_SAVINGS_DEFAULT,
};
