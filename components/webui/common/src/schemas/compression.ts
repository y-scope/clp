import {
    Static,
    Type,
} from "@sinclair/typebox";


/**
 * Schema for an absolute file system path.
 * - Must not be empty.
 * - Must start with "/": the path must be an absolute path.
 */
const AbsolutePathSchema = Type.String({
    minLength: 1,
    pattern: "^/",
});

/**
 * Schema for request to create a new compression job.
 */
const CompressionJobCreationSchema = Type.Object({
    paths: Type.Array(AbsolutePathSchema, {minItems: 1}),
    dataset: Type.Optional(Type.String()),
    timestampKey: Type.Optional(Type.String()),
});

type CompressionJobCreation = Static<typeof CompressionJobCreationSchema>;

/**
 * Schema for compression job response.
 */
const CompressionJobSchema = Type.Object({
    jobId: Type.Number(),
});

type CompressionJob = Static<typeof CompressionJobSchema>;

export {
    AbsolutePathSchema,
    CompressionJobCreationSchema,
    CompressionJobSchema,
};
export type {
    CompressionJob,
    CompressionJobCreation,
};
