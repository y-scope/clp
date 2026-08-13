//! The crate-level error type for the search coordinator.

/// Errors returned by the search coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("spider request failure: {0}")]
    SpiderClient(#[from] spider_client::error::ClientError),

    #[error("sqlx error: {0}")]
    Sqlx(#[from] sqlx::Error),

    #[error("number of search tasks {0} exceeds `i32::MAX`")]
    TooManySearchTasks(usize),
}
