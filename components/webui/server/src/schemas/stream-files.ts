import {Type} from "@sinclair/typebox";

import {QUERY_JOB_TYPE} from "../typings/query.js";
import {StringSchema} from "./common.js";


/**
 * Schema for request to extract a stream file.
 */
const StreamFileExtractionSchema = Type.Object({
    // Type.Null must come before Type.String;
    // otherwise, `{dataset: null}` gets converted to `{dataset: ""}`.
    dataset: Type.Union(
        [
            Type.Null(),
            Type.String(),
        ]
    ),
    extractJobType: Type.Enum(QUERY_JOB_TYPE),
    logEventIdx: Type.Integer(),
    streamId: StringSchema,
});

export {StreamFileExtractionSchema};
