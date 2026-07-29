//! The crate-level error type for the compression coordinator.

use clp_rust_utils::job_config::ingestion::JobId as IngestionJobId;
use clp_rust_utils::s3::S3ObjectMetadataId;

/// Errors returned by the compression coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error(
        "duplicate S3 object metadata IDs {ids:?} requested for ingestion job {ingestion_job_id}"
    )]
    DuplicateS3ObjectMetadata {
        ingestion_job_id: IngestionJobId,
        ids: Vec<S3ObjectMetadataId>,
    },

    #[error("S3 object metadata {id} has an empty `{field}`")]
    EmptyS3ObjectMetadataField {
        id: S3ObjectMetadataId,
        field: &'static str,
    },

    #[error("invalid dataset: {0}")]
    InvalidDataset(String),

    #[error("invalid endpoint: {0}")]
    InvalidEndpoint(String),

    #[error("failed to create metadata table `{table}`: {source}")]
    MetadataTableCreation {
        table: String,
        #[source]
        source: sqlx::Error,
    },

    #[error("missing S3 object metadata {id} for ingestion job {ingestion_job_id}")]
    MissingS3ObjectMetadata {
        ingestion_job_id: IngestionJobId,
        id: S3ObjectMetadataId,
    },

    #[error("no S3 object metadata was requested for ingestion job {0}")]
    NoS3ObjectMetadata(IngestionJobId),

    #[error("no S3 objects were partitioned into compression task inputs")]
    NoTaskInputs,

    #[error("S3 bucket mismatch: expected `{0}`, but got `{1}`")]
    S3BucketMismatch(String, String),

    #[error("S3 key prefix mismatch: expected key to start with `{0}`, but got `{1}`")]
    S3KeyPrefixMismatch(String, String),

    #[error("spider request failure: {0}")]
    SpiderClient(#[from] spider_client::error::ClientError),

    #[error("sqlx error: {0}")]
    Sqlx(#[from] sqlx::Error),

    /// Failed to build or serialize the compression task graph.
    #[error("failed to build the compression task graph: {0}")]
    TaskGraph(#[from] spider_core::task::Error),

    /// Failed to msgpack-serialize a task input.
    #[error("failed to serialize a task input: {0}")]
    TaskInputSerialization(#[from] rmp_serde::encode::Error),

    #[error("number of compression tasks {0} exceeds `i32::MAX`")]
    TooManyCompressionTasks(usize),

    #[error("unsupported input config")]
    UnsupportedInputConfig,
}
