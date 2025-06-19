import {querySql} from "../../../IngestPage/sqlConfig";

/**
 * Table name for dataset queries.
 */
const SqlDbClpDatasetsTableName = "clp_datasets";


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
FROM ${SqlDbClpDatasetsTableName}
ORDER BY ${CLP_DATASETS_TABLE_COLUMN_NAMES.NAME};
`;

interface DatasetItem {
    name: string;
}


/**
 * Fetches all datasets names from the `clp_datasets` table.
 *
 * @return
 */
const fetchDatasetNames = async (): Promise<string[]> => {
    const resp = await querySql<DatasetItem[]>(GET_DATASETS_SQL);
    return resp.data.map(dataset => dataset.name);
};

export type {
    DatasetItem,
};
export {fetchDatasetNames, GET_DATASETS_SQL};
