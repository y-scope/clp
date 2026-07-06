import {
    Static,
    Type,
} from "@sinclair/typebox";

import {
    IdSchema,
    StringSchema,
} from "./common.js";


const MIN_NUM_SEARCH_RESULTS = 1;

const MAX_NUM_SEARCH_RESULTS = 10000;

const DEFAULT_MAX_NUM_SEARCH_RESULTS = 1000;

/**
 * Schema for request to create a new query job.
 */
const QueryJobCreationSchema = Type.Object({
    datasets: Type.Array(Type.String()),
    ignoreCase: Type.Boolean(),
    maxNumResults: Type.Optional(Type.Integer({
        maximum: MAX_NUM_SEARCH_RESULTS,
        minimum: MIN_NUM_SEARCH_RESULTS,
    })),
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
    DEFAULT_MAX_NUM_SEARCH_RESULTS,
    MAX_NUM_SEARCH_RESULTS,
    MIN_NUM_SEARCH_RESULTS,
    QueryJobCreationSchema,
    QueryJobSchema,
};
export type {
    QueryJob,
    QueryJobCreation,
};
