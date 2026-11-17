//! The events of a session's query job, and the errors that end them.

use clp_rust_utils::job_config::QueryJobStatus;
use tokio::task::JoinError;

use crate::session::SessionStats;

/// An event of a session's query job.
///
/// # Type Parameters
///
/// * `BatchType` - The type of a batch of encoded results.
#[derive(Debug)]
pub enum SessionEvent<BatchType> {
    /// A batch of the job's results.
    Results(BatchType),

    /// The end of the events, once the job has terminated and the session's result stream has
    /// ended.
    Ended {
        /// The terminal status of the job.
        status: QueryJobStatus,

        /// The statistics of the session.
        stats: SessionStats,
    },

    /// The end of the events because the session failed or waiting for the job's termination
    /// failed. The job's cancellation has been requested.
    Failed(SessionEventError),
}

/// Reasons the events of a session's query job end in a failure.
#[derive(Debug, thiserror::Error)]
pub enum SessionEventError {
    #[error("the session's results can't be encoded: {0}")]
    Encoding(String),

    #[error("waiting for the query job's termination failed: {0}")]
    Termination(Box<dyn std::error::Error + Send + Sync>),

    #[error("the task waiting for the query job's termination failed: {0}")]
    TerminationTaskFailed(JoinError),
}
