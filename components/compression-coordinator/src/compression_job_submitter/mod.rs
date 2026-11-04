//! The compression-job-submission API trait for driving CLP compression jobs on a Spider
//! (Huntsman) cluster.

mod spider;

use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::CompressionJobId;
use clp_rust_utils::task_io::compression::ClpSCompressionOption;
use clp_rust_utils::task_io::compression::FsInputSource;
use clp_rust_utils::task_io::compression::S3InputSource;
use serde::Deserialize;
use serde::Serialize;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;

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

/// Associates a compression input manifest with its Spider task and input type.
pub trait CompressionManifest: Serialize + Send + Sync + 'static {
    /// The registered TDL task name that consumes this manifest.
    const TASK_FUNC: &'static str;

    /// The manifest's struct name in the task's input type descriptor.
    const TYPE_NAME: &'static str;
}

impl CompressionManifest for S3InputSource {
    const TASK_FUNC: &'static str = "compression::clp_s_s3_compress";
    const TYPE_NAME: &'static str = "S3InputSource";
}

impl CompressionManifest for FsInputSource {
    const TASK_FUNC: &'static str = "compression::clp_s_fs_compress";
    const TYPE_NAME: &'static str = "FsInputSource";
}

/// Drives CLP compression jobs on a Spider (Huntsman) cluster.
#[async_trait]
pub trait CompressionJobSubmitter: Clone + Send + Sync {
    /// Builds the compression task graph for `input_sources` and registers it with Spider, without
    /// starting it.
    ///
    /// # Type Parameters
    ///
    /// * `ManifestType` - The concrete input manifest consumed by each compression task.
    ///
    /// # Parameters
    ///
    /// * `compression_job_id` - The unique ID of the CLP compression job.
    /// * `resource_group_id` - The Spider resource group to register the job under.
    /// * `clp_s_option` - `clp-s` tuning options shared by every task in the job.
    /// * `dataset` - The dataset to compress into.
    /// * `input_sources` - Input manifests to compress, each paired with the execution policy to
    ///   apply to the compression task it creates. Each element represents an input to a
    ///   compression task.
    /// * `commit_task_execution_policy` - The execution policy to apply to the job's commit task.
    ///
    /// # Returns
    ///
    /// The job ID issued by Spider on success.
    ///
    /// # Errors
    ///
    /// Implementations must document their error conditions.
    async fn submit_compression_job<ManifestType: CompressionManifest>(
        &self,
        compression_job_id: CompressionJobId,
        resource_group_id: ResourceGroupId,
        clp_s_option: ClpSCompressionOption,
        dataset: DatasetName,
        input_sources: Vec<(ManifestType, ExecutionPolicy)>,
        commit_task_execution_policy: ExecutionPolicy,
    ) -> Result<JobId, Error>;

    /// Requests that the job identified by `spider_job_id` start and polls its state until
    /// terminal.
    ///
    /// The Spider implementation attempts to start the job on every call and continues polling if
    /// Spider rejects the start because of the job's state.
    ///
    /// # Parameters
    ///
    /// * `spider_job_id` - The job to request a start for and wait on.
    /// * `initial_poll_backoff` - The initial delay between non-terminal job-state polls.
    /// * `max_poll_backoff` - The cap on the delay between job-state polls.
    ///
    /// # Returns
    ///
    /// The job's terminal outcome on success.
    ///
    /// # Errors
    ///
    /// Implementations must document their error conditions.
    async fn run_compression_job_to_completion(
        &self,
        spider_job_id: JobId,
        initial_poll_backoff: Duration,
        max_poll_backoff: Duration,
    ) -> Result<CompressionJobOutcome, Error>;
}
