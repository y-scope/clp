//! The crate-level error type for the compression coordinator.

/// Errors returned by the compression coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("invalid dataset: {0}")]
    InvalidDataset(String),

    #[error("invalid endpoint: {0}")]
    InvalidEndpoint(String),

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

    #[error("unsupported input config")]
    UnsupportedInputConfig,
}
