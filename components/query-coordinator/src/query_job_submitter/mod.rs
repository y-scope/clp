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
    /// Registers, but does not start, one query task per archive.
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
