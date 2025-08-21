import axios from "axios";


/**
 * Query the SQL server with the queryString.
 *
 * @param queryString
 * @return
 */
const querySql = async <T>(queryString: string) => {
    return axios.post<T>("/api/archive-metadata/sql", {queryString});
};

export {querySql};
