//! Registration, execution, and cancellation of Spider jobs.

use std::time::Duration;

use async_trait::async_trait;
use spider_core::task::TaskGraph;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;
use spider_core::types::io::TaskGraphInput;

use crate::Error;

/// A job's terminal outcome.
#[derive(Debug)]
pub enum JobOutcome {
    /// The job completed successfully.
    Succeeded,

    /// The job failed with the given error.
    Failed { error_message: String },

    /// The job was cancelled before reaching completion.
    Cancelled,
}

/// Drives CLP jobs on a Spider (Huntsman) cluster.
#[async_trait]
pub trait SpiderJobSubmitter: Clone + Send + Sync + 'static {
    /// Registers a job with Spider without starting it.
    ///
    /// # Parameters
    ///
    /// * `resource_group_id` - The Spider resource group to register the job under.
    /// * `graph` - The job's task graph.
    /// * `input` - The job's positional task inputs.
    ///
    /// # Returns
    ///
    /// The job ID issued by Spider on success.
    ///
    /// # Errors
    ///
    /// Implementations must document their error conditions.
    async fn submit_job(
        &self,
        resource_group_id: ResourceGroupId,
        graph: &TaskGraph,
        input: &TaskGraphInput,
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
    /// * `poll_interval` - The delay after each non-terminal job-state poll.
    ///
    /// # Returns
    ///
    /// The job's terminal outcome on success.
    ///
    /// # Errors
    ///
    /// Implementations must document their error conditions.
    async fn run_job_to_completion(
        &self,
        spider_job_id: JobId,
        poll_interval: Duration,
    ) -> Result<JobOutcome, Error>;

    /// Idempotently requests cancellation for the job identified by `spider_job_id`.
    ///
    /// Spider can accept a cancellation request before the job finishes running its cleanup task.
    ///
    /// Safe to call regardless of whether the job is not-yet-started, already running, or already
    /// terminal.
    ///
    /// # Parameters
    ///
    /// * `spider_job_id` - The job to cancel.
    ///
    /// # Errors
    ///
    /// Implementations must document their error conditions.
    async fn cancel_job(&self, spider_job_id: JobId) -> Result<(), Error>;
}
