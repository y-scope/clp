import {
    Static,
    Type,
} from "@sinclair/typebox";

import {StringSchema} from "./common.js";


/**
 * Schema for S3 listing request query parameters.
 */
const S3ListRequestSchema = Type.Object({
    bucket: Type.String({minLength: 1}),
    continuationToken: Type.Optional(Type.String()),
    endpointUrl: Type.Optional(Type.String()),
    prefix: Type.Optional(Type.String()),
    regionCode: Type.Optional(Type.String()),
});

type S3ListRequest = Static<typeof S3ListRequestSchema>;

/**
 * Schema for an S3 object or common prefix entry.
 */
const S3EntrySchema = Type.Object({
    isPrefix: Type.Boolean(),
    key: StringSchema,
});

type S3Entry = Static<typeof S3EntrySchema>;

/**
 * Schema for S3 listing response.
 */
const S3ListResponseSchema = Type.Object({
    entries: Type.Array(S3EntrySchema),
    isTruncated: Type.Boolean(),
    nextContinuationToken: Type.Union([Type.String(),
        Type.Null()]),
});

type S3ListResponse = Static<typeof S3ListResponseSchema>;

export {
    S3EntrySchema,
    S3ListRequestSchema,
    S3ListResponseSchema,
};
export type {
    S3Entry,
    S3ListRequest,
    S3ListResponse,
};
