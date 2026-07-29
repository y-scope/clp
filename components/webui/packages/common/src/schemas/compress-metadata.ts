import {
    Static,
    Type,
} from "@sinclair/typebox";

import {ClpIoPartialConfigSchema} from "./compression.js";


/**
 * Base compression metadata fields.
 */
const CompressionMetadataBaseSchema = Type.Object({
    _id: Type.Number(),
    compressed_size: Type.Number(),
    duration: Type.Union([Type.Number(),
        Type.Null()]),
    start_time: Type.Union([Type.String(),
        Type.Null()]),
    status: Type.Number(),
    status_msg: Type.String(),
    uncompressed_size: Type.Number(),
    update_time: Type.String(),
});

/**
 * Compression metadata as stored, including the encoded IO config blob.
 */
const CompressionMetadataSchema = Type.Intersect([
    CompressionMetadataBaseSchema,
    Type.Object({
        clp_config: Type.Unsafe<Buffer>({}),
    }),
]);

type CompressionMetadata = Static<typeof CompressionMetadataSchema>;

/**
 * Compression metadata including decoded IO config.
 */
const CompressionMetadataDecodedSchema = Type.Intersect(
    [
        CompressionMetadataBaseSchema,
        Type.Object({
            clp_config: ClpIoPartialConfigSchema,
        }),
    ]
);

type CompressionMetadataDecoded = Static<typeof CompressionMetadataDecodedSchema>;


export {
    CompressionMetadataDecodedSchema,
    CompressionMetadataSchema,
};
export type {
    CompressionMetadata,
    CompressionMetadataDecoded,
};
