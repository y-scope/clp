//! The query-job submission interface.

mod spider;

use async_trait::async_trait;
use clp_rust_utils::job_config::ArchiveId;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use non_empty_string::NonEmptyString;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;

use crate::Error;

/// Identifies an archive handled by query tasks.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ArchiveMetadata {
    /// The archive's ID.
    pub id: ArchiveId,

    /// The archive's dataset, or `None` for the default dataset.
    pub dataset: Option<NonEmptyString>,

    /// The archive's compressed size in bytes.
    pub size: u64,
}

/// Drives CLP query jobs on a Spider (Huntsman) cluster.
#[async_trait]
pub trait QueryJobSubmitter: Clone + Send + Sync {
    /// Builds the query task graph for the given archives and registers it with Spider, without
    /// starting it.
    ///
    /// # Parameters
    ///
    /// * `query_job_id` - The unique ID of the CLP query job.
    /// * `resource_group_id` - The Spider resource group to register the job under.
    /// * `clp_s_query_options` - `clp-s` query options shared by every task in the job.
    /// * `output_handle` - The output handle selecting how the query outputs are returned.
    /// * `archives_to_search` - The archives to search, each represents a query task paired with
    ///   the task execution policy.
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
        clp_s_query_option: ClpSQueryOption,
        output_handle: OutputHandle,
        archives_to_search: Vec<(ArchiveMetadata, ExecutionPolicy)>,
    ) -> Result<JobId, Error>;
}
