import {Type} from "@sinclair/typebox";

import {StringSchema} from "./common.js";


/**
 * Schema for request to create a new query job.
 */
const PrestoSearchJobCreationSchema = Type.Object({
    queryString: StringSchema,
});

const PrestoJobSchema = Type.Object({
    searchJobId: StringSchema,
});

export {
    PrestoJobSchema,
    PrestoSearchJobCreationSchema,
};
