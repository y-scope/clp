//! The query-job submission interface.

mod spider;

use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use non_empty_string::NonEmptyString;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;

use crate::Error;

/// The terminal outcome of a query job.
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum QueryJobOutcome {
    /// Every archive query completed successfully.
    Succeeded,

    /// At least one archive query failed.
    Failed { error_message: String },

    /// Spider cancelled the job unexpectedly. User-requested cancellation is outside the MVP.
    UnexpectedlyCancelled,
}

/// Registers CLP-S query jobs with a distributed task scheduler.
#[async_trait]
pub trait QueryJobSubmitter: Clone + Send + Sync {
    /// Registers, but does not start, one query task per `(dataset, archive_id)` pair.
    ///
    /// # Errors
    ///
    /// Implementations must document their error conditions.
    async fn submit_query_job(
        &self,
        query_job_id: QueryJobId,
        resource_group_id: ResourceGroupId,
        clp_s_query_option: ClpSQueryOption,
        output_handle: OutputHandle,
        archives: Vec<(Option<NonEmptyString>, NonEmptyString)>,
        query_task_execution_policy: ExecutionPolicy,
    ) -> Result<JobId, Error>;

    /// Idempotently starts `spider_job_id` and waits for it to reach a terminal state.
    ///
    /// # Errors
    ///
    /// Implementations must document their error conditions.
    async fn run_query_job_to_completion(
        &self,
        spider_job_id: JobId,
        initial_poll_backoff: Duration,
        max_poll_backoff: Duration,
    ) -> Result<QueryJobOutcome, Error>;
}
