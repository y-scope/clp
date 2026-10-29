import {
    Static,
    Type,
} from "@sinclair/typebox";

import {StringSchema} from "./common.js";


/**
 * Schema for request to create a new Presto query job.
 */
const PrestoQueryJobCreationSchema = Type.Object({
    queryString: StringSchema,
});

/**
 * Schema to identify a Presto query job.
 */
const PrestoQueryJobSchema = Type.Object({
    searchJobId: StringSchema,
});

type PrestoQueryJobCreation = Static<typeof PrestoQueryJobCreationSchema>;

type PrestoQueryJob = Static<typeof PrestoQueryJobSchema>;


export {
    PrestoQueryJobCreationSchema,
    PrestoQueryJobSchema,
};
export type {
    PrestoQueryJob,
    PrestoQueryJobCreation,
};
