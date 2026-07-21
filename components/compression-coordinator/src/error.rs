//! The crate-level error type for the compression coordinator.

/// Errors returned by the compression coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("sqlx error: {0}")]
    Sqlx(#[from] sqlx::Error),

    #[error("invalid dataset: {0}")]
    InvalidDataset(String),

    #[error("unsupported input config")]
    UnsupportedInputConfig,
}
