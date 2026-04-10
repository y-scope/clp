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
    ingestion_job_id: Type.Optional(Type.Union([Type.Number(),
        Type.Null()])),
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
            s3_paths: Type.Optional(Type.Array(Type.String())),
        }),
    ]
);

type CompressionMetadataDecoded = Static<typeof CompressionMetadataDecodedSchema>;


/**
 * Matching the `ClpIngestionJobStatus` enum in the log-ingestor.
 */
enum IngestionJobStatus {
    Requested = "requested",
    Running = "running",
    Paused = "paused",
    Failed = "failed",
    Finished = "finished",
}

/**
 * Matching the `ingestion_job` table schema created by the log-ingestor.
 */
const IngestionJobSchema = Type.Object({
    _id: Type.Number(),
    config: Type.String(),
    creation_ts: Type.String(),
    last_update_ts: Type.String(),
    num_files_compressed: Type.Number(),
    status: Type.Union([
        Type.Literal(IngestionJobStatus.Requested),
        Type.Literal(IngestionJobStatus.Running),
        Type.Literal(IngestionJobStatus.Paused),
        Type.Literal(IngestionJobStatus.Failed),
        Type.Literal(IngestionJobStatus.Finished),
    ]),
});

type IngestionJob = Static<typeof IngestionJobSchema>;

/**
 * Combined response schema for the compress-metadata route, including both
 * compression jobs (with optional ingestion_job_id) and ingestion jobs.
 */
const JobsResponseSchema = Type.Object({
    compressionJobs: Type.Array(CompressionMetadataDecodedSchema),
    ingestionJobs: Type.Array(IngestionJobSchema),
});

type JobsResponse = Static<typeof JobsResponseSchema>;


export {
    CompressionMetadataDecodedSchema,
    CompressionMetadataSchema,
    IngestionJobSchema,
    IngestionJobStatus,
    JobsResponseSchema,
};
export type {
    CompressionMetadata,
    CompressionMetadataDecoded,
    IngestionJob,
    JobsResponse,
};
