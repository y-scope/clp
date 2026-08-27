//! [`QueryJobSubmitter`] skeleton for [`spider_client::SpiderClient`].

use async_trait::async_trait;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use spider_client::SpiderClient;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;

use crate::Error;
use crate::query_job_submitter::QueryJobSubmitter;

#[async_trait]
impl QueryJobSubmitter for SpiderClient {
    /// # Errors
    ///
    /// Task-graph construction and submission are not implemented yet.
    async fn submit_query_job(
        &self,
        query_job_id: QueryJobId,
        resource_group_id: ResourceGroupId,
        clp_s_query_option: ClpSQueryOption,
        archives: Vec<(String, String)>,
        query_task_execution_policy: ExecutionPolicy,
    ) -> Result<JobId, Error> {
        let _ = (
            query_job_id,
            resource_group_id,
            clp_s_query_option,
            archives,
            query_task_execution_policy,
        );
        todo!("Construct and submit the CLP-S query task graph")
    }
}
