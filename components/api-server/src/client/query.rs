//! Query job configuration and lifecycle operations.

use std::num::NonZeroU32;

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

use super::Client;
use super::ClientError;

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

/// Defines the request configuration for submitting a query job whose results are streamed instead
/// of buffered.
#[derive(Debug, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamingQueryConfig {
    /// The query to run.
    pub query: SearchQuery,
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
        let api_server_config = self.get_api_server_config();
        let mut delay_ms = api_server_config.query_job_polling.initial_backoff_ms;
        let max_delay_ms = api_server_config.query_job_polling.max_backoff_ms;
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
                    tokio::time::sleep(tokio::time::Duration::from_millis(delay_ms)).await;
                    delay_ms = std::cmp::min(delay_ms.saturating_mul(2), max_delay_ms);
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
}
