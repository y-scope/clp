import {querySql} from "../../../../../api/sql";
import {settings} from "../../../../../settings";


/**
 * Builds a SQL query string to retrieve the total uncompressed bytes and duration
 * for a specific query job within a given dataset.
 *
 * @param datasetName
 * @param jobId
 * @return
 */
const buildQuerySpeedSql = (datasetName: string, jobId: string) => {
    const tableName = "" === datasetName ?
        settings.SqlDbClpArchivesTableName :
        `${settings.SqlDbClpTablePrefix}${datasetName}_archives`;

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
    JOIN ${tableName} ca
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
 * within a given dataset by executing a SQL query.
 *
 * @param datasetName
 * @param jobId
 * @return
 */
const fetchQuerySpeed = async (datasetName: string, jobId: string): Promise<QuerySpeedResp> => {
    const resp = await querySql<QuerySpeedResp[]>(buildQuerySpeedSql(datasetName, jobId));
    const [data] = resp.data;
    if ("undefined" === typeof data) {
        throw new Error("Invalid query speed.");
    }

    return data;
};

export {fetchQuerySpeed};
