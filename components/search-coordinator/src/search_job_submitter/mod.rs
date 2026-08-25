//! The query-job-submission API trait for driving CLP search jobs on a Spider (Huntsman) cluster.

mod spider;

use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::job_config::QueryJobId;
use serde::Deserialize;
use serde::Serialize;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;

use crate::error::Error;

/// The terminal outcome of a query job.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum QueryJobOutcome {
    /// The job completed successfully.
    Succeeded,

    /// The job failed with the given error.
    Failed { error_message: String },

    /// The job was cancelled before reaching completion.
    Cancelled,
}

/// Drives CLP search jobs on a Spider (Huntsman) cluster.
#[async_trait]
pub trait QueryJobSubmitter: Clone + Send + Sync {
    /// Builds the search task graph and registers it with Spider, without starting it.
    ///
    /// # Parameters
    ///
    /// * `query_job_id` - The unique ID of the CLP query job.
    /// * `resource_group_id` - The Spider resource group to register the job under.
    /// * `dataset` - The dataset to search, if any.
    ///
    /// # Returns
    ///
    /// The job ID issued by Spider on success.
    ///
    /// # Errors
    ///
    /// Implementations must document their error conditions.
    async fn submit_query_job(
        &self,
        query_job_id: QueryJobId,
        resource_group_id: ResourceGroupId,
        dataset: Option<String>,
    ) -> Result<JobId, Error>;

    /// Idempotently starts the job identified by `spider_job_id` (only if it hasn't already been
    /// started) and waits until it reaches a terminal state.
    ///
    /// Safe to call regardless of whether the job is not-yet-started, already running, or already
    /// terminal.
    ///
    /// # Parameters
    ///
    /// * `spider_job_id` - The job to start (if needed) and wait on.
    /// * `initial_poll_backoff` - The delay before the first job-state poll.
    /// * `max_poll_backoff` - The cap on the delay between job-state polls.
    ///
    /// # Returns
    ///
    /// The job's terminal outcome on success.
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
