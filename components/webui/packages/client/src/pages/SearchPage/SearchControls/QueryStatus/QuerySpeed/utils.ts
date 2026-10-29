import {querySql} from "../../../../../api/sql";
import {settings} from "../../../../../settings";


/**
 * Builds a SQL query string to retrieve the total uncompressed bytes and duration
 * for a specific query job across one or more datasets.
 *
 * @param datasetNames
 * @param jobId
 * @return
 */
const buildQuerySpeedSql = (datasetNames: string[], jobId: string) => {
    let archivesSubquery: string;
    if (0 === datasetNames.length) {
        archivesSubquery = "SELECT id, uncompressed_size" +
            ` FROM ${settings.SqlDbClpArchivesTableName}`;
    } else if (1 === datasetNames.length) {
        archivesSubquery = "SELECT id, uncompressed_size" +
            ` FROM ${settings.SqlDbClpTablePrefix}${datasetNames[0]}_archives`;
    } else {
        archivesSubquery = datasetNames
            .map((name) => "SELECT id, uncompressed_size" +
                ` FROM ${settings.SqlDbClpTablePrefix}${name}_archives`)
            .join(" UNION ALL ");
    }

    return `WITH qt AS (
    SELECT job_id, archive_id
    FROM query_tasks
    WHERE
        archive_id IS NOT NULL
        AND job_id = ${jobId}
),
totals AS (
    SELECT
        qt.job_id,
        SUM(ca.uncompressed_size) AS total_uncompressed_bytes
    FROM qt
    JOIN (${archivesSubquery}) ca
    ON qt.archive_id = ca.id
)
SELECT
    CAST(totals.total_uncompressed_bytes AS double) AS bytes,
    qj.duration AS duration
FROM query_jobs qj
JOIN totals
ON totals.job_id = qj.id`;
};

interface QuerySpeedResp {
    bytes: number | null;
    duration: number | null;
}

/**
 * Fetches the query speed data (bytes and duration) for a specific job ID
 * across the given datasets by executing a SQL query.
 *
 * @param datasetNames
 * @param jobId
 * @return
 */
const fetchQuerySpeed = async (datasetNames: string[], jobId: string): Promise<QuerySpeedResp> => {
    const resp = await querySql<QuerySpeedResp[]>(buildQuerySpeedSql(datasetNames, jobId));
    const [data] = resp.data;
    if ("undefined" === typeof data) {
        throw new Error("Invalid query speed.");
    }

    return data;
};

export {fetchQuerySpeed};
