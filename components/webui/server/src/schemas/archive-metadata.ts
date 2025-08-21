import {Type} from "@sinclair/typebox";

import {StringSchema} from "./common.js";


/**
 * Schema for SQL query request.
 */
const SqlSchema = Type.Object({
    queryString: StringSchema,
});

export {SqlSchema};
