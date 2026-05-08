import {
    Static,
    Type,
} from "@sinclair/typebox";

import {
    IdSchema,
    StringSchema,
} from "./common.js";


/**
 * Schema for request to create a new query job.
 */
const QueryJobCreationSchema = Type.Object({
    datasets: Type.Array(Type.String()),
    ignoreCase: Type.Boolean(),
    queryString: StringSchema,
    timeRangeBucketSizeMillis: Type.Integer(),
    timestampBegin: Type.Union([Type.Null(),
        Type.Integer()]),
    timestampEnd: Type.Union([Type.Null(),
        Type.Integer()]),
});

/**
 * Schema to identify query job.
 */
const QueryJobSchema = Type.Object({
    searchJobId: IdSchema,
    aggregationJobId: IdSchema,
});

type QueryJobCreation = Static<typeof QueryJobCreationSchema>;

type QueryJob = Static<typeof QueryJobSchema>;

export {
    QueryJobCreationSchema,
    QueryJobSchema,
};
export type {
    QueryJob,
    QueryJobCreation,
};
