import axios from "axios";
import { Static } from '@sinclair/typebox'
import {
    SqlSchema,
} from "@webui/common/schemas/archive-metadata"

type Sql = Static<typeof SqlSchema>;


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
