import {querySql} from "../../../IngestPage/sqlConfig";

/**
 * Table names for dataset queries.
 */
enum DATASET_SQL_CONFIG {
    SqlDbClpDatasetsTableName = "clp_datasets",
}

/**
 * Column names for the `clp_datasets` table.
 */
enum CLP_DATASETS_TABLE_COLUMN_NAMES {
    NAME = "name",
}

/**
 * SQL query to get all datasets.
 */
const GET_DATASETS_SQL = `
SELECT
    ${CLP_DATASETS_TABLE_COLUMN_NAMES.NAME} AS name
FROM ${DATASET_SQL_CONFIG.SqlDbClpDatasetsTableName}
ORDER BY ${CLP_DATASETS_TABLE_COLUMN_NAMES.NAME};
`;

interface DatasetItem {
    name: string;
}

/**
 * Fetches all datasets from the database.
 *
 * @return Promise resolving to array of dataset names
 */
const fetchDatasets = async (): Promise<string[]> => {
    const resp = await querySql<DatasetsResp>(GET_DATASETS_SQL);
    return resp.data.map(dataset => dataset.name);
};

/**
 * Fetches all datasets from the database (raw response).
 *
 * @return Promise resolving to array of dataset names
 */
const fetchDatasetsRaw = async (): Promise<string[]> => {
    const resp = await querySql<DatasetItem[]>(GET_DATASETS_SQL);
    return resp.data.map(dataset => dataset.name);
};

export type {
    DatasetItem,
};
export {fetchDatasets, fetchDatasetsRaw, GET_DATASETS_SQL};
