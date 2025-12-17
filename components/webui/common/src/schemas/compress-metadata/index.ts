import {
    Static,
    Type,
} from "@sinclair/typebox";


/**
 * Schema for compression job metadata without decoded config fields.
 */
const CompressionJobBaseSchema = Type.Object({
    _id: Type.Number(),
    compressed_size: Type.Number(),
    duration: Type.Union([Type.Number(), Type.Null()]),
    retrieval_time: Type.Number(),
    start_time: Type.Union([Type.String(), Type.Null()]),
    status: Type.Number(),
    status_msg: Type.String(),
    uncompressed_size: Type.Number(),
    update_time: Type.String(),
});

/**
 * Schema for compression job metadata with decoded config.
 */
const CompressionJobWithDecodedIoConfigSchema = Type.Intersect([
    CompressionJobBaseSchema,
    Type.Object({
        dataset: Type.Union([Type.String(), Type.Null()]),
        paths: Type.Array(Type.String()),
    }),
]);

type CompressionJobBase = Static<typeof CompressionJobBaseSchema>;
type CompressionJobWithDecodedIoConfig = Static<
    typeof CompressionJobWithDecodedIoConfigSchema
>;


export {
    CompressionJobBaseSchema,
    CompressionJobWithDecodedIoConfigSchema,
};
export type {
    CompressionJobBase,
    CompressionJobWithDecodedIoConfig,
};
