//! [`QueryJobSubmitter`] implementation for [`spider_client::SpiderClient`].

use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::job_config::QueryJobId;
use spider_client::SpiderClient;
use spider_client::error::ClientError;
use spider_core::job::JobState;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;

use crate::error::Error;
use crate::search_job_submitter::QueryJobOutcome;
use crate::search_job_submitter::QueryJobSubmitter;

#[async_trait]
impl QueryJobSubmitter for SpiderClient {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Search task-graph construction or job submission fails (not yet implemented).
    async fn submit_query_job(
        &self,
        query_job_id: QueryJobId,
        resource_group_id: ResourceGroupId,
        dataset: Option<String>,
    ) -> Result<JobId, Error> {
        // NOTE: The search task graph (TDL task func, task-input types, and archive-batch inputs)
        // is not yet defined. Unlike compression's `submit_s3_compression_job`, search does not
        // take a `ClpSCompressionOption`, S3 `input_sources`, or a commit-task execution policy.
        let _ = (query_job_id, resource_group_id, dataset);
        todo!("Search task-graph construction and Spider submission are not yet implemented.")
    }

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`SpiderClient::start_job`]'s return values on failure, except
    ///   [`ClientError::InvalidJobState`], which indicates the job has already been started.
    /// * Forwards [`SpiderClient::get_job_state`]'s return values on failure.
    async fn run_query_job_to_completion(
        &self,
        spider_job_id: JobId,
        initial_poll_backoff: Duration,
        max_poll_backoff: Duration,
    ) -> Result<QueryJobOutcome, Error> {
        /// The multiplier applied to the poll backoff after each non-terminal poll.
        const POLL_BACKOFF_FACTOR: u32 = 2;

        match self.start_job(spider_job_id).await {
            Ok(_) | Err(ClientError::InvalidJobState(_)) => {}
            Err(error) => return Err(error.into()),
        }

        let mut backoff = initial_poll_backoff.min(max_poll_backoff);
        let terminal_state = loop {
            let state = self.get_job_state(spider_job_id).await?;
            if state.is_terminal() {
                break state;
            }
            tokio::time::sleep(backoff).await;
            backoff = backoff
                .saturating_mul(POLL_BACKOFF_FACTOR)
                .min(max_poll_backoff);
        };

        Ok(match terminal_state {
            JobState::Succeeded => QueryJobOutcome::Succeeded,
            JobState::Failed => QueryJobOutcome::Failed {
                error_message: self
                    .get_job_error(spider_job_id)
                    .await
                    .unwrap_or_else(|error| format!("<failed to fetch job error: {error}>")),
            },
            JobState::Cancelled => QueryJobOutcome::Cancelled,
            _ => unreachable!(),
        })
    }
}
