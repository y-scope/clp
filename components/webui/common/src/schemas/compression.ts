import {
    Static,
    Type,
} from "@fastify/type-provider-typebox";


/**
 * Schema for request to create a new compression job.
 */
const CompressionJobCreationSchema = Type.Object({
    paths: Type.Array(Type.String()),
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
    CompressionJobCreationSchema,
    CompressionJobSchema,
};
export type {
    CompressionJob,
    CompressionJobCreation,
};
