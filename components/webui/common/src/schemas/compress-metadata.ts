import {
    Static,
    Type,
} from "@sinclair/typebox";


/**
 * Base compression metadata fields.
 */
const CompressionMetadataBaseSchema = Type.Object({
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

type CompressionMetadataBase = Static<typeof CompressionMetadataBaseSchema>;

/**
 * Encoded IO config stored alongside compression metadata.
 */
const CompressionMetadataEncodedConfigSchema = Type.Object({
    clp_config: Type.Optional(Type.Any()),
});

/**
 * Compression metadata as stored, including the encoded IO config.
 */
const CompressionMetadataSchema = Type.Intersect([
    CompressionMetadataBaseSchema,
    CompressionMetadataEncodedConfigSchema,
]);

type CompressionMetadata = CompressionMetadataBase & {clp_config?: Buffer | null};

/**
 * Decoded IO config fields extracted from the encoded config.
 */
const CompressionMetadataIoConfigSchema = Type.Object({
    dataset: Type.Union([Type.String(), Type.Null()]),
    paths: Type.Array(Type.String()),
});

type CompressionMetadataIoConfig = Static<typeof CompressionMetadataIoConfigSchema>;

/**
 * Compression metadata including decoded IO config but excluding the encoded blob.
 */
const CompressionMetadataDecodedSchema = Type.Intersect([
    CompressionMetadataBaseSchema,
    CompressionMetadataIoConfigSchema,
]);

type CompressionMetadataDecoded = Static<typeof CompressionMetadataDecodedSchema>;


export {
    CompressionMetadataBaseSchema,
    CompressionMetadataDecodedSchema,
    CompressionMetadataIoConfigSchema,
    CompressionMetadataSchema,
};
export type {
    CompressionMetadata,
    CompressionMetadataBase,
    CompressionMetadataDecoded,
    CompressionMetadataIoConfig,
};
