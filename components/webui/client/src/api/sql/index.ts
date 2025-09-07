import {Sql} from "@webui/common/schemas/archive-metadata";
import axios from "axios";


/**
 * Query the SQL server with the queryString.
 *
 * @param payload
 * @return
 */
const querySql = async <T>(payload: Sql) => {
    return axios.post<T>("/api/archive-metadata/sql", payload);
};

export {querySql};
