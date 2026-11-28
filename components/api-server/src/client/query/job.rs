//! Submission, cancellation and status of query jobs.

use std::num::NonZeroU32;
use std::ops::ControlFlow;

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
use crate::job_polling;

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
    /// * Forwards [`job_polling::wait_for_status`]'s return values on failure.
    pub(super) async fn wait_for_query_job_termination(
        &self,
        query_job_id: u64,
    ) -> Result<QueryJobStatus, ClientError> {
        job_polling::wait_for_status(
            &self.sql_pool,
            query_job_id,
            &self.get_api_server_config().query_job_polling,
            ClientError::QueryJobNotFound,
            query_termination_poll_result,
        )
        .await
    }

    /// Waits until a query succeeds, fails, or begins cancellation.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`job_polling::wait_for_status`]'s return values on failure.
    pub(super) async fn wait_for_query_success(
        &self,
        query_job_id: u64,
    ) -> Result<(), ClientError> {
        job_polling::wait_for_status(
            &self.sql_pool,
            query_job_id,
            &self.get_api_server_config().query_job_polling,
            ClientError::QueryJobNotFound,
            query_success_poll_result,
        )
        .await
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

/// # Returns
///
/// A terminal status to stop polling, or an instruction to keep polling.
const fn query_termination_poll_result(
    status: QueryJobStatus,
) -> ControlFlow<Result<QueryJobStatus, ClientError>> {
    if status.is_terminal() {
        ControlFlow::Break(Ok(status))
    } else {
        ControlFlow::Continue(())
    }
}

/// # Returns
///
/// A result to stop polling when the query finishes or begins cancellation, or an instruction to
/// keep polling.
const fn query_success_poll_result(status: QueryJobStatus) -> ControlFlow<Result<(), ClientError>> {
    match status {
        QueryJobStatus::Succeeded => ControlFlow::Break(Ok(())),
        QueryJobStatus::Failed
        | QueryJobStatus::CancelRequested
        | QueryJobStatus::Cancelling
        | QueryJobStatus::Cancelled
        | QueryJobStatus::Killed => ControlFlow::Break(Err(ClientError::QueryNotSucceeded)),
        QueryJobStatus::Running | QueryJobStatus::Pending => ControlFlow::Continue(()),
    }
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroU32;
    use std::ops::ControlFlow;

    use clp_rust_utils::job_config::Aggregation;
    use clp_rust_utils::job_config::QueryJobStatus as Status;
    use clp_rust_utils::job_config::QueryOutput;
    use clp_rust_utils::job_config::ResultsCacheOutput;

    use super::QueryOutputConfig;
    use super::query_success_poll_result;
    use super::query_termination_poll_result;
    use crate::error::ClientError;

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
    fn query_termination_poll_result_maps_every_status() {
        for (status, expected) in [
            (Status::Pending, ControlFlow::Continue(())),
            (Status::Running, ControlFlow::Continue(())),
            (Status::Succeeded, ControlFlow::Break(Status::Succeeded)),
            (Status::Failed, ControlFlow::Break(Status::Failed)),
            (Status::CancelRequested, ControlFlow::Continue(())),
            (Status::Cancelling, ControlFlow::Continue(())),
            (Status::Cancelled, ControlFlow::Break(Status::Cancelled)),
            (Status::Killed, ControlFlow::Break(Status::Killed)),
        ] {
            assert_eq!(
                query_termination_poll_result(status)
                    .map_break(|result| result.expect("the terminal status should be returned")),
                expected,
                "status: {status:?}"
            );
        }
    }

    #[test]
    fn query_success_poll_result_maps_every_status() {
        let not_succeeded = ControlFlow::Break(Err(()));
        for (status, expected) in [
            (Status::Pending, ControlFlow::Continue(())),
            (Status::Running, ControlFlow::Continue(())),
            (Status::Succeeded, ControlFlow::Break(Ok(()))),
            (Status::Failed, not_succeeded),
            (Status::CancelRequested, not_succeeded),
            (Status::Cancelling, not_succeeded),
            (Status::Cancelled, not_succeeded),
            (Status::Killed, not_succeeded),
        ] {
            let result = query_success_poll_result(status).map_break(|result| {
                result.map_err(|error| {
                    assert!(
                        matches!(error, ClientError::QueryNotSucceeded),
                        "status {status:?} returned {error:?}"
                    );
                })
            });
            assert_eq!(result, expected, "status: {status:?}");
        }
    }
}
