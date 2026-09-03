//! The crate-level error type for the query coordinator.

/// Errors returned by the query coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("query job {0} is no longer pending")]
    JobNotPending(clp_rust_utils::job_config::QueryJobId),

    #[error("spider request failure: {0}")]
    SpiderClient(#[from] spider_client::error::ClientError),

    #[error("sqlx error: {0}")]
    Sqlx(#[from] sqlx::Error),

    #[error("failed to persist terminal status for query job {query_job_id}: {source}")]
    TerminalStatusPersistence {
        /// The query job whose terminal state could not be persisted.
        query_job_id: clp_rust_utils::job_config::QueryJobId,

        /// The persistence failure.
        #[source]
        source: sqlx::Error,
    },

    #[error("number of query tasks {0} exceeds `i32::MAX`")]
    TooManyQueryTasks(usize),
}
