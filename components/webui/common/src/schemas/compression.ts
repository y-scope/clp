import {
    Static,
    Type,
} from "@fastify/type-provider-typebox";


/**
 * Schema for request to create a new compression job.
 */
const CompressionJobSchema = Type.Object({
    paths: Type.Array(Type.String()),
    dataset: Type.Optional(Type.String()),
    timestampKey: Type.Optional(Type.String()),
});

type CompressionJobSchemaStatic = Static<typeof CompressionJobSchema>;

/**
 * Schema for compression job response.
 */
const CompressionJobCreationSchema = Type.Object({
    jobId: Type.Number(),
});

type CompressionJobCreationSchemaStatic = Static<typeof CompressionJobCreationSchema>;

export {
    CompressionJobCreationSchema,
    CompressionJobSchema,
};
export type {
    CompressionJobCreationSchemaStatic,
    CompressionJobSchemaStatic,
};
