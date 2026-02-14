import {
    CLP_STORAGE_ENGINES,
    SqlTableSuffix,
} from "@webui/common/config";
import {Nullable} from "@webui/common/utility-types";
import dayjs, {Dayjs} from "dayjs";

import {querySql} from "../../../../api/sql";
import {SETTINGS_STORAGE_ENGINE} from "../../../../config";
import {settings} from "../../../../settings";
import {CLP_ARCHIVES_TABLE_COLUMN_NAMES} from "../../../IngestPage/sqlConfig";
import {DEFAULT_TIME_RANGE} from "./utils";


/**
 * Builds a SQL query string to retrieve the minimum and maximum timestamps from the CLP archives
 * table.
 *
 * @return
 */
const buildClpTimeRangeSql = () => `SELECT
    MIN(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.BEGIN_TIMESTAMP})   AS begin_timestamp,
    MAX(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP})     AS end_timestamp
FROM ${settings.SqlDbClpArchivesTableName}
`;

/**
 * Builds a SQL query string to retrieve the minimum and maximum timestamps across multiple CLP-s
 * datasets' archives using UNION ALL.
 *
 * @param datasetNames
 * @return
 */
const buildClpsTimeRangeSql = (datasetNames: string[]): string => {
    const unionParts = datasetNames.map((name) => `SELECT
  MIN(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.BEGIN_TIMESTAMP}) AS begin_timestamp,
  MAX(${CLP_ARCHIVES_TABLE_COLUMN_NAMES.END_TIMESTAMP}) AS end_timestamp
FROM ${settings.SqlDbClpTablePrefix}${name}_${SqlTableSuffix.ARCHIVES}`);

    return `SELECT
  MIN(begin_timestamp) AS begin_timestamp,
  MAX(end_timestamp) AS end_timestamp
FROM (${unionParts.join("\nUNION ALL\n")}) AS combined`;
};

/**
 * Fetches the earliest and latest log entry timestamps ("all time" range)
 * from the configured storage engine (CLP or CLPS).
 *
 * @param selectDatasets
 * @return
 */
const fetchAllTimeRange = async (selectDatasets: string[]): Promise<[Dayjs, Dayjs]> => {
    let sql: string;
    if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
        sql = buildClpTimeRangeSql();
    } else {
        if (0 === selectDatasets.length) {
            console.error("Cannot fetch \"All Time\" time range. No selected datasets.");

            return DEFAULT_TIME_RANGE;
        }
        sql = buildClpsTimeRangeSql(selectDatasets);
    }
    const resp = await querySql<
        {
            begin_timestamp: Nullable<number>;
            end_timestamp: Nullable<number>;
        }[]
    >(sql);
    const [timestamps] = resp.data;
    if ("undefined" === typeof timestamps ||
              null === timestamps.begin_timestamp ||
              null === timestamps.end_timestamp
    ) {
        return DEFAULT_TIME_RANGE;
    }

    return [
        dayjs.utc(timestamps.begin_timestamp),
        dayjs.utc(timestamps.end_timestamp),
    ];
};

export {fetchAllTimeRange};
