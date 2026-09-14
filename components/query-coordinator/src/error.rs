//! The crate-level error type for the query coordinator.

/// Errors returned by the query coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("invalid query job configuration: {0}")]
    InvalidQueryJobConfig(String),

    #[error("query job {0} is no longer pending")]
    JobNotPending(clp_rust_utils::job_config::QueryJobId),

    #[error("no archives were partitioned into query task inputs")]
    NoTaskInputs,

    #[error("spider request failure: {0}")]
    SpiderClient(#[from] spider_client::error::ClientError),

    #[error("sqlx error: {0}")]
    Sqlx(#[from] sqlx::Error),

    #[error("number of query tasks {0} exceeds `i32::MAX`")]
    TooManyQueryTasks(usize),
}
