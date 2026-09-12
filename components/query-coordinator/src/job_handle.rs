//! Lifecycle management for one coordinator-planned query job.

use std::num::NonZeroU32;
use std::sync::Arc;
use std::time::Duration;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::SearchJobConfig;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use const_format::formatcp;
use non_empty_string::NonEmptyString;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId as SpiderJobId;
use spider_core::types::id::ResourceGroupId;
use sqlx::MySqlPool;

use crate::Error;
use crate::query_job_submitter::ArchiveMetadata;
use crate::query_job_submitter::QueryJobOutcome;
use crate::query_job_submitter::QueryJobSubmitter;

/// Options for a query job running in Spider.
pub struct SpiderOption {
    pub initial_poll_backoff: Duration,
    pub max_poll_backoff: Duration,
}

/// Drives one already-planned query job through submission and terminal persistence.
///
/// # Type Parameters
///
/// * `SubmitterType` - The type of the job submitter for Spider job submission.
pub struct QueryJobHandle<SubmitterType: QueryJobSubmitter> {
    db_pool: MySqlPool,
    _db_config: Database,
    query_job_id: QueryJobId,
    job_submitter: SubmitterType,
    resource_group_id: ResourceGroupId,
    _search_job_config: SearchJobConfig,
    clp_s_query_option: ClpSQueryOption,
    output_handle: OutputHandle,
    spider_option: Arc<SpiderOption>,
}

impl<SubmitterType: QueryJobSubmitter> QueryJobHandle<SubmitterType> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created [`QueryJobHandle`] for the given query job configuration.
    ///
    /// # Errors
    ///
    /// Returns an error if the query string is empty.
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        db_pool: MySqlPool,
        db_config: Database,
        query_job_id: QueryJobId,
        job_submitter: SubmitterType,
        resource_group_id: ResourceGroupId,
        search_job_config: SearchJobConfig,
        output_handle: OutputHandle,
        spider_option: Arc<SpiderOption>,
    ) -> Result<Self, Error> {
        let query_string = NonEmptyString::try_from(search_job_config.query_string.clone())
            .map_err(|_| {
                Error::InvalidQueryJobConfig("query string must not be empty".to_owned())
            })?;
        let clp_s_query_option = ClpSQueryOption {
            query_string,
            max_num_results: NonZeroU32::new(search_job_config.max_num_results),
            begin_timestamp_millisecs: search_job_config.begin_timestamp,
            end_timestamp_millisecs: search_job_config.end_timestamp,
            ignore_case: search_job_config.ignore_case,
        };

        Ok(Self {
            db_pool,
            _db_config: db_config,
            query_job_id,
            job_submitter,
            resource_group_id,
            _search_job_config: search_job_config,
            clp_s_query_option,
            output_handle,
            spider_option,
        })
    }

    /// Submits the prepared graph and drives the query job to a terminal state.
    ///
    /// On a submission failure, this method makes a best-effort attempt to mark the CLP query job
    /// as failed before returning the original error. After the job is durably running, monitoring
    /// and terminal-persistence failures leave it running so recovery can reattach to Spider.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::submit`]'s return values on failure.
    /// * Forwards [`Self::to_completion`]'s return values on failure.
    pub async fn run(self) -> Result<(), Error> {
        tracing::info!(query_job_id = % self.query_job_id, "Starting query job.");

        let spider_job_id = match self.submit().await {
            Ok(spider_job_id) => spider_job_id,
            Err(error) => {
                if !matches!(error, Error::JobNotPending(_)) {
                    self.report_failure(&error).await;
                }
                return Err(error);
            }
        };
        self.to_completion(spider_job_id).await
    }

    /// Resumes a query job that was already submitted to Spider.
    ///
    /// The caller must ensure `spider_job_id` belongs to this CLP query job.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::to_completion`]'s return values on failure.
    pub async fn recover(self, spider_job_id: SpiderJobId) -> Result<(), Error> {
        tracing::info!(
            query_job_id = % self.query_job_id,
            spider_job_id = % spider_job_id,
            "Recovering query job.",
        );

        self.to_completion(spider_job_id).await
    }

    /// Submits the query job to Spider and persists its running state.
    ///
    /// # Returns
    ///
    /// The submitted Spider job ID on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::TooManyQueryTasks`] if the number of query tasks exceeds `i32`'s range.
    /// * Forwards [`QueryJobSubmitter::submit_query_job`]'s return values on failure.
    /// * Forwards [`Self::persist_spider_job_id`]'s return values on failure.
    async fn submit(&self) -> Result<SpiderJobId, Error> {
        let archives_to_search = self.prepare_task_inputs().await?;
        let num_tasks = archives_to_search.len();
        if num_tasks == 0 {
            return Err(Error::NoArchivesToSearch);
        }
        let persisted_num_tasks =
            i32::try_from(num_tasks).map_err(|_| Error::TooManyQueryTasks(num_tasks))?;
        let spider_job_id = self
            .job_submitter
            .submit_query_job(
                self.query_job_id,
                self.resource_group_id,
                self.clp_s_query_option.clone(),
                self.output_handle.clone(),
                archives_to_search,
            )
            .await?;

        tracing::info!(
            query_job_id = % self.query_job_id,
            spider_job_id = % spider_job_id,
            num_tasks,
            "Query job submitted.",
        );

        self.persist_spider_job_id(spider_job_id, persisted_num_tasks)
            .await?;
        Ok(spider_job_id)
    }

    /// Prepares the archive inputs and execution policies for the query tasks.
    ///
    /// # Returns
    ///
    /// The archives to search and their execution policies on success.
    ///
    /// # Errors
    ///
    /// Returns an error if archive input preparation fails.
    async fn prepare_task_inputs(&self) -> Result<Vec<(ArchiveMetadata, ExecutionPolicy)>, Error> {
        todo!("prepare query task inputs")
    }

    /// Persists the Spider job ID and marks the query job as running.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::JobNotPending`] if the query job is no longer pending.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn persist_spider_job_id(
        &self,
        spider_job_id: SpiderJobId,
        num_tasks: i32,
    ) -> Result<(), Error> {
        let query = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `spider_id` = ?, `status` = ?, `num_tasks` = ?, \
             `start_time` = CURRENT_TIMESTAMP(3) WHERE `id` = ? AND `status` = ?"
        );
        let result = sqlx::query(query)
            .bind(spider_job_id.get())
            .bind(QueryJobStatus::Running)
            .bind(num_tasks)
            .bind(self.query_job_id)
            .bind(QueryJobStatus::Pending)
            .execute(&self.db_pool)
            .await?;

        if 1 != result.rows_affected() {
            return Err(Error::JobNotPending(self.query_job_id));
        }
        Ok(())
    }

    /// Waits for the associated Spider job to complete and finalizes the query job.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::update_job_status`]'s return values on failure.
    /// * Forwards [`QueryJobSubmitter::run_query_job_to_completion`]'s return values on failure.
    async fn to_completion(&self, spider_job_id: SpiderJobId) -> Result<(), Error> {
        let outcome = self
            .job_submitter
            .run_query_job_to_completion(
                spider_job_id,
                self.spider_option.initial_poll_backoff,
                self.spider_option.max_poll_backoff,
            )
            .await?;

        tracing::info!(
            query_job_id = % self.query_job_id,
            spider_job_id = % spider_job_id,
            outcome = ? outcome,
            "Query job reached a terminal Spider state.",
        );

        let (status, status_message) = match outcome {
            QueryJobOutcome::Succeeded => (QueryJobStatus::Succeeded, None),
            QueryJobOutcome::Failed { error_message } => (
                QueryJobStatus::Failed,
                Some(format!("The Spider query job failed: {error_message}")),
            ),
        };
        self.update_job_status(status, status_message.as_deref(), QueryJobStatus::Running)
            .await?;
        Ok(())
    }

    /// Reports a query job orchestration failure.
    ///
    /// Logs the original error and makes a best-effort attempt to mark the query job as failed. If
    /// terminal-status persistence fails, the status-update error is logged and otherwise ignored.
    async fn report_failure(&self, error: &Error) {
        tracing::error!(
            query_job_id = % self.query_job_id,
            error = % error,
            "Query job orchestration failed.",
        );

        let _ = self
            .update_job_status(
                QueryJobStatus::Failed,
                Some(&format!("Query job orchestration failed: {error}")),
                QueryJobStatus::Pending,
            )
            .await
            .inspect_err(|status_error| {
                tracing::error!(
                    query_job_id = % self.query_job_id,
                    error = % status_error,
                    "Failed to persist the query job failure.",
                );
            });
    }

    /// Updates a query job only when it has the expected non-terminal status.
    /// Leaves the status message unchanged when `status_message` is `None`.
    /// A zero-row update is treated as success so an ineligible or missing job row is left
    /// unchanged.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn update_job_status(
        &self,
        status: QueryJobStatus,
        status_message: Option<&str>,
        expected_status: QueryJobStatus,
    ) -> Result<(), sqlx::Error> {
        let query = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = COALESCE(LEFT(?, \
             512), `status_msg`), `duration` = CASE WHEN `start_time` IS NULL THEN 0 ELSE \
             TIMESTAMPDIFF(MICROSECOND, `start_time`, CURRENT_TIMESTAMP(3)) / 1000000.0 END WHERE \
             `id` = ? AND `status` = ?"
        );
        let query = sqlx::query(query)
            .bind(status)
            .bind(status_message)
            .bind(self.query_job_id)
            .bind(expected_status);
        query.execute(&self.db_pool).await?;
        Ok(())
    }
}
