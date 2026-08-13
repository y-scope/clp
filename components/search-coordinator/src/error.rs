//! The crate-level error type for the search coordinator.

/// Errors returned by the search coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("invalid configuration: {0}")]
    InvalidConfiguration(String),

    #[error("invalid endpoint: {0}")]
    InvalidEndpoint(String),

    #[error("spider request failure: {0}")]
    SpiderClient(#[from] spider_client::error::ClientError),

    #[error("sqlx error: {0}")]
    Sqlx(#[from] sqlx::Error),

    #[error("semaphore error: {0}")]
    Semaphore(String),

    #[error("number of search tasks {0} exceeds `i32::MAX`")]
    TooManySearchTasks(usize),

    #[error("unsupported input config")]
    UnsupportedInputConfig,
}
