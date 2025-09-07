import {
    Static,
    Type,
} from "@sinclair/typebox";

import {StringSchema} from "./common.js";


/**
 * Schema for SQL query request.
 */
const SqlSchema = Type.Object({
    queryString: StringSchema,
});

type Sql = Static<typeof SqlSchema>;

export {SqlSchema};
export type {Sql};
