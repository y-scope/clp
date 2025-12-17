import {
    Static,
    Type,
} from "@sinclair/typebox";
import {ClpIoConfig, ClpIoConfigSchema} from "./compression.js";


/**
 * Base compression metadata fields (internal only).
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

/**
 * Compression metadata as stored, including the encoded IO config blob.
 */
const CompressionMetadataSchema = Type.Intersect([
    CompressionMetadataBaseSchema,
    Type.Object({
        clp_config: Type.Optional(ClpIoConfigSchema),
    }),
]);

type CompressionMetadata = Omit<
    Static<typeof CompressionMetadataSchema>,
    "clp_config"
> & {clp_config?: Buffer | null};

/**
 * Decoded IO config fields extracted from the encoded config.
 */
const DecodedIoConfigSchema = Type.Object({
    clp_config: Type.Optional(ClpIoConfigSchema),
});

type DecodedIoConfig = {clp_config?: ClpIoConfig | null};

/**
 * Compression metadata including decoded IO config but excluding the encoded blob.
 */
const CompressionMetadataDecodedSchema = Type.Intersect(
    [
        CompressionMetadataBaseSchema,
        DecodedIoConfigSchema,
    ]
);

type CompressionMetadataDecoded = Static<typeof CompressionMetadataDecodedSchema>;


export {
    CompressionMetadataDecodedSchema,
    CompressionMetadataSchema,
    DecodedIoConfigSchema,
};
export type {
    CompressionMetadata,
    CompressionMetadataDecoded,
    DecodedIoConfig,
};
