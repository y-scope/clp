//! The crate-level error type for the compression coordinator.

/// Errors returned by the compression coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("S3 bucket mismatch: expected `{0}`, but got `{1}`")]
    S3BucketMismatch(String, String),
}
