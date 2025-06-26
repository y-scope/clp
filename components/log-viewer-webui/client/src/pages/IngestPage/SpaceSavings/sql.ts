import {
    CLP_ARCHIVES_TABLE_COLUMN_NAMES,
} from "../sqlConfig";


/**
 * Builds the query string to query space savings stats for multiple datasets.
 *
 * @param datasetNames
 * @return
 */
const buildMultiDatasetSpaceSavingsSql = (datasetNames: string[]): string => {
    const archiveQueries = datasetNames.map((name) => `
    SELECT
        ${CLP_ARCHIVES_TABLE_COLUMN_NAMES.UNCOMPRESSED_SIZE},
        ${CLP_ARCHIVES_TABLE_COLUMN_NAMES.SIZE}
    FROM clp_${name}_archives
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

interface SpaceSavingsItem {
    total_uncompressed_size: number;
    total_compressed_size: number;
}

/**
 * Default state for space savings.
 */
const SPACE_SAVINGS_DEFAULT: SpaceSavingsItem = {
    total_compressed_size: 0,
    total_uncompressed_size: 0,
};

type SpaceSavingsResp = SpaceSavingsItem[];

export type {
    SpaceSavingsItem,
    SpaceSavingsResp,
};
export {
    buildMultiDatasetSpaceSavingsSql,
    SPACE_SAVINGS_DEFAULT,
};
