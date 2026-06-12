import {querySql} from "../../../../api/sql";
import {settings} from "../../../../settings";


/**
 * Column names for the datasets table.
 */
enum CLP_DATASETS_TABLE_COLUMN_NAMES {
    NAME = "name",
}

/**
 * SQL query to get all dataset names.
 */
const GET_DATASETS_SQL = `
    SELECT
        ${CLP_DATASETS_TABLE_COLUMN_NAMES.NAME} AS name
    FROM ${settings.SqlDbClpDatasetsTableName}
    UNION
    SELECT DISTINCT
        dataset AS name
    FROM hot_log_segments
    WHERE 0 < committed_end_offset
    ORDER BY ${CLP_DATASETS_TABLE_COLUMN_NAMES.NAME};
`;

interface DatasetItem {
    [CLP_DATASETS_TABLE_COLUMN_NAMES.NAME]: string;
}

/**
 * Fetches all dataset names from the datasets table.
 *
 * @return
 */
const fetchDatasetNames = async (): Promise<string[]> => {
    const resp = await querySql<DatasetItem[]>(GET_DATASETS_SQL);
    return resp.data.map((dataset) => dataset.name);
};

export {fetchDatasetNames};
