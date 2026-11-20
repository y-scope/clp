//! Submission, cancellation and status of query jobs.

use std::num::NonZeroU32;
use std::time::Duration;

use clp_rust_utils::clp_config::package::config::QueryJobPollingConfig;
use clp_rust_utils::database::mysql::query_job;
use clp_rust_utils::job_config::Aggregation;
use clp_rust_utils::job_config::Cancellability;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobConfig;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::ResultsCacheOutput;
use clp_rust_utils::job_config::SearchQuery;
use serde::Deserialize;
use serde::Serialize;
use sqlx::Row;
use utoipa::ToSchema;

use crate::client::Client;
use crate::error::ClientError;

/// Defines the request configuration for submitting a query job.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct QueryConfig {
    /// The query to run.
    pub query: SearchQuery,

    /// Where to store the query results.
    pub output: QueryOutputConfig,
}

impl From<QueryConfig> for QueryJobConfig {
    fn from(config: QueryConfig) -> Self {
        Self {
            query: config.query,
            output: config.output.into(),
        }
    }
}

/// Defines where a submitted query job's results are buffered.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
#[serde(deny_unknown_fields, tag = "type", rename_all = "snake_case")]
pub enum QueryOutputConfig {
    /// Buffers the results in the results cache (backed by `MongoDB`).
    ResultsCache {
        /// The maximum number of results to keep. Omit for no limit.
        #[serde(default)]
        #[schema(value_type = Option<u32>, minimum = 1)]
        max_num_results: Option<NonZeroU32>,

        /// The aggregation to compute over all matching results, regardless of `max_num_results`.
        #[serde(default)]
        aggregation: Option<Aggregation>,
    },

    /// Buffers the results in files stored in the stream output storage.
    File {},
}

impl From<QueryOutputConfig> for QueryOutput {
    fn from(output: QueryOutputConfig) -> Self {
        match output {
            QueryOutputConfig::ResultsCache {
                max_num_results,
                aggregation,
            } => Self::ResultsCache(ResultsCacheOutput {
                max_num_results,
                aggregation,
            }),
            QueryOutputConfig::File {} => Self::File {},
        }
    }
}

impl Client {
    /// Submits a search or aggregation query as a job.
    ///
    /// # Returns
    ///
    /// The unique ID of the newly created query job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`query_job::submit`]'s return values on failure.
    pub async fn submit_query(
        &self,
        query_job_config: &QueryJobConfig,
    ) -> Result<u64, ClientError> {
        Ok(query_job::submit(&self.sql_pool, query_job_config).await?)
    }

    /// Submits a cancellation request for a query job.
    ///
    /// This method is idempotent: if a cancellation has already been requested for the job, it's a
    /// no-op that returns `Ok(())`.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::QueryJobNotFound`] if the job doesn't exist.
    /// * [`ClientError::QueryJobAlreadyTerminated`] if the job has already succeeded, failed, or
    ///   been killed.
    /// * Forwards [`query_job::request_cancellation`]'s return values on failure.
    pub async fn cancel_query_job(&self, query_job_id: u64) -> Result<(), ClientError> {
        match query_job::request_cancellation(&self.sql_pool, query_job_id).await? {
            None => Err(ClientError::QueryJobNotFound(query_job_id)),
            Some(Cancellability::Cancellable | Cancellability::CancellationRequested) => Ok(()),
            Some(Cancellability::NotCancellable) => {
                Err(ClientError::QueryJobAlreadyTerminated(query_job_id))
            }
        }
    }

    /// Retrieves the status of a previously submitted query job.
    ///
    /// # Returns
    ///
    /// The current status of the job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::QueryJobNotFound`] if the job doesn't exist.
    /// * Forwards [`query_job::get_status`]'s return values on failure.
    pub async fn get_status(&self, query_job_id: u64) -> Result<QueryJobStatus, ClientError> {
        query_job::get_status(&self.sql_pool, query_job_id)
            .await?
            .ok_or(ClientError::QueryJobNotFound(query_job_id))
    }

    /// Waits until a query job terminates, reading its status with the backoff of the API server's
    /// query job polling configuration.
    ///
    /// # Returns
    ///
    /// The terminal status of the job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Client::get_status`]'s return values on failure.
    pub(super) async fn wait_for_query_job_termination(
        &self,
        query_job_id: u64,
    ) -> Result<QueryJobStatus, ClientError> {
        let mut backoff =
            QueryJobPollingBackoff::new(&self.get_api_server_config().query_job_polling);
        loop {
            let status = self.get_status(query_job_id).await?;
            if status.is_terminal() {
                return Ok(status);
            }
            tokio::time::sleep(backoff.next_delay()).await;
        }
    }

    /// Waits until a query succeeds, fails, or begins cancellation.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::QueryNotSucceeded`] if the query fails or cancellation has begun.
    /// * Forwards [`Self::get_status`]'s return values on failure.
    pub(super) async fn wait_for_query_success(
        &self,
        query_job_id: u64,
    ) -> Result<(), ClientError> {
        let mut backoff =
            QueryJobPollingBackoff::new(&self.get_api_server_config().query_job_polling);
        loop {
            match self.get_status(query_job_id).await? {
                QueryJobStatus::Succeeded => {
                    break;
                }
                QueryJobStatus::Failed
                | QueryJobStatus::CancelRequested
                | QueryJobStatus::Cancelling
                | QueryJobStatus::Cancelled
                | QueryJobStatus::Killed => {
                    return Err(ClientError::QueryNotSucceeded);
                }
                QueryJobStatus::Running | QueryJobStatus::Pending => {
                    tokio::time::sleep(backoff.next_delay()).await;
                }
            }
        }

        Ok(())
    }

    pub(super) async fn get_job_config(
        &self,
        query_job_id: u64,
    ) -> Result<QueryJobConfig, ClientError> {
        let row = sqlx::query(&format!(
            "SELECT job_config FROM `{QUERY_JOBS_TABLE_NAME}` WHERE id = ?"
        ))
        .bind(query_job_id)
        .fetch_optional(&self.sql_pool)
        .await?
        .ok_or(ClientError::QueryJobNotFound(query_job_id))?;
        let msgpack: &[u8] = row.try_get("job_config")?;
        Ok(rmp_serde::from_slice(msgpack)?)
    }

    /// # Returns
    ///
    /// A reference to the API server configuration.
    ///
    /// # Panics
    ///
    /// Panics if `self.config.api_server` is `None`.
    const fn get_api_server_config(
        &self,
    ) -> &clp_rust_utils::clp_config::package::config::ApiServer {
        self.config
            .api_server
            .as_ref()
            .expect("api_server configuration is missing")
    }
}

/// The backoff between the reads of a query job's status: the first delay is the configured initial
/// backoff, and each next delay doubles the previous one, up to the configured maximum backoff.
struct QueryJobPollingBackoff {
    delay: Duration,
    max_delay: Duration,
}

impl QueryJobPollingBackoff {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created backoff that `config` configures, starting at its first delay.
    const fn new(config: &QueryJobPollingConfig) -> Self {
        Self {
            delay: Duration::from_millis(config.initial_backoff_ms),
            max_delay: Duration::from_millis(config.max_backoff_ms),
        }
    }

    /// # Returns
    ///
    /// The delay before the next read of the status. The delay after it is doubled, up to the
    /// maximum.
    fn next_delay(&mut self) -> Duration {
        let delay = self.delay;
        self.delay = std::cmp::min(self.delay.saturating_mul(2), self.max_delay);
        delay
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn query_output_config_converts_to_the_matching_query_output() {
        use clp_rust_utils::types::PositiveI64;

        let max_num_results = NonZeroU32::new(7);
        let aggregation = Some(Aggregation::CountByTime {
            bucket_size_millisecs: PositiveI64::new(60_000).expect("60,000 is positive"),
        });
        for (output, expected) in [
            (
                QueryOutputConfig::ResultsCache {
                    max_num_results,
                    aggregation,
                },
                QueryOutput::ResultsCache(ResultsCacheOutput {
                    max_num_results,
                    aggregation,
                }),
            ),
            (QueryOutputConfig::File {}, QueryOutput::File {}),
        ] {
            assert_eq!(
                QueryOutput::from(output.clone()),
                expected,
                "output: {output:?}"
            );
        }
    }

    #[test]
    fn query_job_polling_backoff_doubles_its_delay_up_to_the_maximum() {
        let mut backoff = QueryJobPollingBackoff::new(&QueryJobPollingConfig {
            initial_backoff_ms: 100,
            max_backoff_ms: 500,
        });

        for expected_delay_ms in [100, 200, 400, 500, 500] {
            assert_eq!(
                backoff.next_delay(),
                Duration::from_millis(expected_delay_ms)
            );
        }
    }
}
