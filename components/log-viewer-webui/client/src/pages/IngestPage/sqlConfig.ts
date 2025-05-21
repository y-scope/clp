import axios from "axios";


/**
 * Sql table names.
 */
const SQL_CONFIG = Object.freeze({
    SqlDbClpArchivesTableName: "clp_archives",
    SqlDbClpFilesTableName: "clp_files",
    SqlDbCompressionJobsTableName: "compression_jobs",
});

/**
 * Query the SQL server with the queryString.
 *
 * @param queryString
 * @return
 */
const querySql = async <T>(queryString:string) => {
    return axios.post<T>("/query/sql", {queryString});
};

export {
    querySql, SQL_CONFIG,
};
