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
    // Type.Null must come before Type.String;
    // otherwise, `{dataset: null}` gets converted to `{dataset: ""}`.
    dataset: Type.Union([Type.Null(),
        Type.String()]),
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
