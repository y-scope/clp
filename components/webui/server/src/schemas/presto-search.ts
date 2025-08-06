import {Type} from "@sinclair/typebox";

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

export {
    PrestoQueryJobCreationSchema,
    PrestoQueryJobSchema,
};
