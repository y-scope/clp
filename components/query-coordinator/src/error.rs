//! The crate-level error type for the query coordinator.

/// Errors returned by the query coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("invalid coordinator configuration: {0}")]
    InvalidConfiguration(String),

    #[error("invalid Spider endpoint: {0}")]
    InvalidEndpoint(String),

    #[error("semaphore error: {0}")]
    Semaphore(String),

    #[error("invalid query job configuration: {0}")]
    InvalidQueryJobConfig(String),

    #[error("query job {0} is no longer pending")]
    JobNotPending(clp_rust_utils::job_config::QueryJobId),

    #[error("spider request failure: {0}")]
    SpiderClient(#[from] spider_client::error::ClientError),

    #[error("sqlx error: {0}")]
    Sqlx(#[from] sqlx::Error),

    #[error("number of query tasks {0} exceeds `i32::MAX`")]
    TooManyQueryTasks(usize),

    #[error("no archives were selected for the query job")]
    NoArchivesToSearch,

    #[error("failed to build the query task graph: {0}")]
    TaskGraph(#[from] spider_core::task::Error),

    #[error("failed to serialize a task input: {0}")]
    TaskInputSerialization(#[from] rmp_serde::encode::Error),
}
