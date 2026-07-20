//! The compression-job-submission API trait for driving CLP S3 compression jobs on a Spider
//! (Huntsman) cluster.

mod spider;

use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::{
    job_config::CompressionJobId,
    task_io::compression::{ClpSCompressionOption, S3InputSource},
};
use serde::{Deserialize, Serialize};
use spider_core::types::id::{JobId, ResourceGroupId};

use crate::error::Error;

/// The terminal outcome of a compression job.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum CompressionJobOutcome {
    /// The job completed successfully.
    Succeeded,

    /// The job failed with the given error.
    Failed { error_message: String },

    /// The job was cancelled before reaching completion.
    Cancelled,
}

/// Drives CLP S3 compression jobs on a Spider (Huntsman) cluster.
#[async_trait]
pub trait S3CompressionJobSubmitter: Clone + Send + Sync {
    /// Builds the compression task graph for `input_sources` and registers it with Spider, without
    /// starting it.
    ///
    /// # Parameters
    ///
    /// * `compression_job_id` - The unique ID of the CLP compression job.
    /// * `resource_group_id` - The Spider resource group to register the job under.
    /// * `clp_s_option` - `clp-s` tuning options shared by every task in the job.
    /// * `dataset` - The dataset to compress into.
    /// * `input_sources` - S3 input sources to compress. Each represents an input to a compression
    ///   task.
    ///
    /// # Returns
    ///
    /// The job ID issued by Spider on success.
    ///
    /// # Errors
    ///
    /// Implementations must document their error conditions.
    async fn submit_s3_compression_job(
        &self,
        compression_job_id: CompressionJobId,
        resource_group_id: ResourceGroupId,
        clp_s_option: ClpSCompressionOption,
        dataset: Option<String>,
        input_sources: Vec<S3InputSource>,
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
    async fn run_s3_compression_job_to_completion(
        &self,
        spider_job_id: JobId,
        initial_poll_backoff: Duration,
        max_poll_backoff: Duration,
    ) -> Result<CompressionJobOutcome, Error>;
}
