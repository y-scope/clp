//! [`QueryJobSubmitter`] skeleton for [`spider_client::SpiderClient`].

use async_trait::async_trait;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use spider_client::SpiderClient;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;

use crate::Error;
use crate::query_job_submitter::ArchiveMetadata;
use crate::query_job_submitter::QueryJobSubmitter;

#[async_trait]
impl QueryJobSubmitter for SpiderClient {
    /// # Errors
    ///
    /// Task-graph construction and submission are not implemented yet.
    async fn submit_query_job(
        &self,
        _query_job_id: QueryJobId,
        _resource_group_id: ResourceGroupId,
        _clp_s_query_option: ClpSQueryOption,
        _output_handle: OutputHandle,
        _archives_to_search: Vec<(ArchiveMetadata, ExecutionPolicy)>,
    ) -> Result<JobId, Error> {
        todo!("Construct and submit the CLP-S query task graph")
    }
}
