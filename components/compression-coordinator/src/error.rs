//! The crate-level error type for the compression coordinator.

/// Errors returned by the compression coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("S3 object metadata {metadata_id} has an empty {field}")]
    EmptyS3ObjectMetadataField {
        metadata_id: u64,
        field: &'static str,
    },

    #[error("invalid dataset: {0}")]
    InvalidDataset(String),

    #[error("failed to create metadata table `{table}`: {source}")]
    MetadataTableCreation {
        table: String,
        #[source]
        source: sqlx::Error,
    },

    #[error(
        "missing S3 object metadata for ingestion job {ingestion_job_id}; missing IDs: \
         {metadata_ids:?}"
    )]
    MissingS3ObjectMetadata {
        ingestion_job_id: u64,
        metadata_ids: Vec<u64>,
    },

    #[error("no S3 object metadata was requested for ingestion job {0}")]
    NoS3ObjectMetadata(u64),

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
