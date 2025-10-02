import {querySql} from "../../../../../../api/sql";
import {SqlTableSuffix} from "../../../../../../config/sql-table-suffix";
import {settings} from "../../../../../../settings";


/**
 * Column names for the column metadata table.
 */
enum CLP_COLUMN_METADATA_TABLE_COLUMN_NAMES {
    NAME = "name",
    TYPE = "type",
}

/**
 * Matching the `NodeType::DateString` value in
 * `clp/components/core/src/clp_s/SchemaTree.hpp`.
 */
const TIMESTAMP_TYPE = 8;

interface TimestampColumnItem {
    [CLP_COLUMN_METADATA_TABLE_COLUMN_NAMES.NAME]: string;
}

/**
 * Builds SQL query to get timestamp columns for a specific dataset.
 *
 * @param datasetName
 * @return
 */
const buildTimestampColumnsSql = (datasetName: string): string => `
    SELECT DISTINCT
        ${CLP_COLUMN_METADATA_TABLE_COLUMN_NAMES.NAME}
    FROM ${settings.SqlDbClpTablePrefix}${datasetName}_${SqlTableSuffix.COLUMN_METADATA}
    WHERE ${CLP_COLUMN_METADATA_TABLE_COLUMN_NAMES.TYPE} = ${TIMESTAMP_TYPE}
    ORDER BY ${CLP_COLUMN_METADATA_TABLE_COLUMN_NAMES.NAME};
`;

/**
 * Fetches timestamp column names for a specific dataset.
 *
 * @param datasetName
 * @return
 */
const fetchTimestampColumns = async (datasetName: string): Promise<string[]> => {
    const sql = buildTimestampColumnsSql(datasetName);
    const resp = await querySql<TimestampColumnItem[]>(sql);
    return resp.data.map((column) => column.name);
};

export {fetchTimestampColumns};
