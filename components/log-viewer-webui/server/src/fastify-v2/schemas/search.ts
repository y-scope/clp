import {Type} from "@sinclair/typebox";

import {
    IdSchema,
    StringSchema,
} from "./common.js";


export const CreateSearchJobSchema = Type.Object({
    queryString: StringSchema,
    timestampBegin: Type.Integer(),
    timestampEnd: Type.Integer(),
    ignoreCase: Type.Boolean(),
    timeRangeBucketSizeMillis: Type.Integer(),
});

export const SearchJobSchema = Type.Object({
    searchJobId: IdSchema,
    aggregationJobId: IdSchema,
});
