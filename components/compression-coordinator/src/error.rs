//! The crate-level error type for the compression coordinator.

/// Errors returned by the compression coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("S3 bucket mismatch: expected `{0}`, but got `{1}`")]
    S3BucketMismatch(String, String),

    /// Failed to build or serialize the compression task graph.
    #[error("failed to build the compression task graph: {0}")]
    TaskGraph(#[from] spider_core::task::Error),

    /// Failed to msgpack-serialize a task input.
    #[error("failed to serialize a task input: {0}")]
    TaskInputSerialization(#[from] rmp_serde::encode::Error),

    #[error("spider request failure: {0}")]
    SpiderClient(#[from] spider_client::error::ClientError),
}
