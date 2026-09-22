//! The crate-level error type for the query coordinator.

use clp_rust_utils::job_config::QueryJobId;

/// Errors returned by the query coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("invalid query job configuration: {0}")]
    InvalidQueryJobConfig(String),

    #[error("metadata corrupted for query job {0}: its status changed or its row no longer exists")]
    QueryJobMetadataCorrupted(QueryJobId),

    #[error("spider request failure: {0}")]
    SpiderClient(#[from] spider_client::error::ClientError),

    #[error("sqlx error: {0}")]
    Sqlx(#[from] sqlx::Error),

    #[error("number of query tasks {0} exceeds `i32::MAX`")]
    TooManyQueryTasks(usize),

    #[error("failed to build the query task graph: {0}")]
    TaskGraph(#[from] spider_core::task::Error),

    #[error("failed to serialize a task input: {0}")]
    TaskInputSerialization(#[from] rmp_serde::encode::Error),
}
