//! [`QueryJobSubmitter`] implementation for [`spider_client::SpiderClient`].

use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use spider_client::SpiderClient;
use spider_client::error::ClientError;
use spider_core::job::JobState;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;

use crate::Error;
use crate::query_job_submitter::ArchiveMetadata;
use crate::query_job_submitter::QueryJobOutcome;
use crate::query_job_submitter::QueryJobSubmitter;

#[async_trait]
impl QueryJobSubmitter for SpiderClient {
    async fn submit_query_job(
        &self,
        _query_job_id: QueryJobId,
        _resource_group_id: ResourceGroupId,
        _clp_s_query_option: ClpSQueryOption,
        _output_handle: OutputHandle,
        _archives_to_search: Vec<(ArchiveMetadata, ExecutionPolicy)>,
    ) -> Result<JobId, Error> {
        todo!("construct and submit the clp-s query task graph")
    }

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`SpiderClient::start_job`]'s return values on failure, except
    ///   [`ClientError::InvalidJobState`].
    /// * Forwards [`SpiderClient::get_job_state`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if Spider returns a terminal state without a corresponding [`QueryJobOutcome`].
    async fn run_query_job_to_completion(
        &self,
        spider_job_id: JobId,
        initial_poll_backoff: Duration,
        max_poll_backoff: Duration,
    ) -> Result<QueryJobOutcome, Error> {
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
            JobState::Failed => {
                let error_message = match self.get_job_error(spider_job_id).await {
                    Ok(error_message) => error_message,
                    Err(error) => {
                        tracing::warn!(
                            spider_job_id = % spider_job_id,
                            error = % error,
                            "Failed to fetch the Spider job error.",
                        );
                        format!("<failed to fetch job error: {error}>")
                    }
                };
                QueryJobOutcome::Failed { error_message }
            }
            JobState::Cancelled => QueryJobOutcome::UnexpectedlyCancelled,
            _ => unreachable!("a terminal Spider state must have a terminal outcome"),
        })
    }
}
