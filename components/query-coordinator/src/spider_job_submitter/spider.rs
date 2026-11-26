//! [`SpiderJobSubmitter`] implementation for [`SpiderClient`].

use std::time::Duration;

use async_trait::async_trait;
use spider_client::SpiderClient;
use spider_client::error::ClientError;
use spider_core::job::JobState;
use spider_core::task::TaskGraph;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;
use spider_core::types::io::TaskGraphInput;

use super::JobOutcome;
use super::SpiderJobSubmitter;
use crate::Error;

#[async_trait]
impl SpiderJobSubmitter for SpiderClient {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`SpiderClient::submit_job`]'s return values on failure.
    async fn submit_job(
        &self,
        resource_group_id: ResourceGroupId,
        graph: &TaskGraph,
        input: &TaskGraphInput,
    ) -> Result<JobId, Error> {
        Ok(Self::submit_job(self, resource_group_id, graph, input).await?)
    }

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`SpiderClient::start_job`]'s return values on failure, except
    ///   [`ClientError::InvalidJobState`], after which the method still polls for a terminal state.
    /// * Forwards [`SpiderClient::get_job_state`]'s return values on failure.
    async fn run_job_to_completion(
        &self,
        spider_job_id: JobId,
        poll_interval: Duration,
    ) -> Result<JobOutcome, Error> {
        match self.start_job(spider_job_id).await {
            Ok(_) | Err(ClientError::InvalidJobState(_)) => {}
            Err(error) => return Err(error.into()),
        }

        let terminal_state = loop {
            let state = self.get_job_state(spider_job_id).await?;
            if state.is_terminal() {
                break state;
            }
            tokio::time::sleep(poll_interval).await;
        };

        Ok(match terminal_state {
            JobState::Succeeded => JobOutcome::Succeeded,
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
                JobOutcome::Failed { error_message }
            }
            JobState::Cancelled => JobOutcome::Cancelled,
            _ => unreachable!("a terminal Spider state must have a terminal outcome"),
        })
    }

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`SpiderClient::cancel_job`]'s return values on failure, except
    ///   [`ClientError::InvalidJobState`], which indicates the job is already being cancelled or
    ///   has already terminated.
    async fn cancel_job(&self, spider_job_id: JobId) -> Result<(), Error> {
        match Self::cancel_job(self, spider_job_id).await {
            Ok(_) | Err(ClientError::InvalidJobState(_)) => Ok(()),
            Err(error) => Err(error.into()),
        }
    }
}
