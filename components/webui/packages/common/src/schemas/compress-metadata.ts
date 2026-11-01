import {
    Static,
    Type,
} from "@sinclair/typebox";


/**
 * Display fields of a compression job config's `input` section. Fields that a job didn't store are
 * omitted.
 */
const CompressionMetadataDisplayInputSchema = Type.Object({
    dataset: Type.Optional(Type.String()),
    keys: Type.Optional(Type.Array(Type.String())),
    path_prefix_to_remove: Type.Optional(Type.String()),
    paths_to_compress: Type.Optional(Type.Array(Type.String())),
    timestamp_key: Type.Optional(Type.String()),
    type: Type.Optional(Type.String()),
    unstructured: Type.Optional(Type.Boolean()),
});

/**
 * Display fields of a compression job config's `output` section. Fields that a job didn't store
 * are omitted.
 */
const CompressionMetadataDisplayOutputSchema = Type.Object({
    compression_level: Type.Optional(Type.Integer()),
    target_archive_size: Type.Optional(Type.Integer()),
    target_dictionaries_size: Type.Optional(Type.Integer()),
    target_encoded_file_size: Type.Optional(Type.Integer()),
    target_segment_size: Type.Optional(Type.Integer()),
});

/**
 * Display fields of a compression job config.
 */
const CompressionMetadataDisplayConfigSchema = Type.Object({
    input: CompressionMetadataDisplayInputSchema,
    output: CompressionMetadataDisplayOutputSchema,
});

type CompressionMetadataDisplayConfig = Static<typeof CompressionMetadataDisplayConfigSchema>;

/**
 * Compression job metadata, with the display fields of the job's config.
 */
const CompressionMetadataSchema = Type.Object({
    _id: Type.Integer(),
    clp_config: CompressionMetadataDisplayConfigSchema,
    compressed_size: Type.Integer(),
    duration: Type.Union([Type.Number(),
        Type.Null()]),
    start_time: Type.Union([Type.String(),
        Type.Null()]),
    status: Type.Integer(),
    status_msg: Type.String(),
    uncompressed_size: Type.Integer(),
    update_time: Type.String(),
});

type CompressionMetadata = Static<typeof CompressionMetadataSchema>;


export {CompressionMetadataSchema};
export type {
    CompressionMetadata,
    CompressionMetadataDisplayConfig,
};
