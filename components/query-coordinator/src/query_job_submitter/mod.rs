//! The query-job submission interface.

mod spider;

use async_trait::async_trait;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;

use crate::Error;

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
        archives: Vec<(String, String)>,
        query_task_execution_policy: ExecutionPolicy,
    ) -> Result<JobId, Error>;
}
