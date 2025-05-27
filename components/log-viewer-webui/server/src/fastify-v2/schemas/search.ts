import {Type} from "@sinclair/typebox";

import {
    IdSchema,
    StringSchema,
} from "./common.js";


/**
 * Schema for request to create a new query job.
 */
const QueryJobCreationSchema = Type.Object({
    ignoreCase: Type.Boolean(),
    queryString: StringSchema,
    timeRangeBucketSizeMillis: Type.Integer(),
    timestampBegin: Type.Integer(),
    timestampEnd: Type.Integer(),
});

/**
 * Schema to identify query job.
 */
const QueryJobSchema = Type.Object({
    searchJobId: IdSchema,
    aggregationJobId: IdSchema,
});

export {
    QueryJobCreationSchema,
    QueryJobSchema,
};
