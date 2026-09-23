//! Handle for driving a single query job to completion.

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
use crate::plan::PlanningOption;
use crate::query_job_submitter::ArchiveMetadata;
use crate::query_job_submitter::QueryJobOutcome;
use crate::query_job_submitter::QueryJobSubmitter;

/// Options for a query job running in Spider.
pub struct SpiderOption {
    pub poll_interval: Duration,
}

/// Resources shared by query job handles created by the coordinator.
pub struct QueryJobHandleContext {
    pub db_pool: MySqlPool,
    pub db_config: Database,
    pub planning_option: PlanningOption,
    pub spider_option: SpiderOption,
}

/// Handles the asynchronous submission of a query job and the retrieval of its result.
///
/// # Type Parameters
///
/// * `SubmitterType` - The type of the job submitter for Spider job submission.
pub struct QueryJobHandle<SubmitterType: QueryJobSubmitter> {
    context: Arc<QueryJobHandleContext>,
    query_job_id: QueryJobId,
    job_submitter: SubmitterType,
    resource_group_id: ResourceGroupId,
    query_job_config: SearchJobConfig,
    clp_s_query_option: ClpSQueryOption,
    output_handle: OutputHandle,
}

impl<SubmitterType: QueryJobSubmitter> QueryJobHandle<SubmitterType> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created [`QueryJobHandle`] for the given query job, with the `clp-s`
    /// query options derived from `search_job_config`.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::InvalidQueryJobConfig`] if the query string is empty.
    pub fn new(
        context: Arc<QueryJobHandleContext>,
        query_job_id: QueryJobId,
        job_submitter: SubmitterType,
        resource_group_id: ResourceGroupId,
<<<<<<< HEAD
        query_job_config: SearchJobConfig,
=======
        search_job_config: SearchJobConfig,
        output_handle: OutputHandle,
>>>>>>> query-coordinator/job-handle
    ) -> Result<Self, Error> {
        let query_string = NonEmptyString::try_from(query_job_config.query_string.clone())
            .map_err(|_| {
                Error::InvalidQueryJobConfig("query string must not be empty".to_owned())
            })?;
        let clp_s_query_option = ClpSQueryOption {
            query_string,
            max_num_results: NonZeroU32::new(query_job_config.max_num_results),
            begin_timestamp_millisecs: query_job_config.begin_timestamp,
            end_timestamp_millisecs: query_job_config.end_timestamp,
            ignore_case: query_job_config.ignore_case,
        };

        Ok(Self {
            context,
            query_job_id,
            job_submitter,
            resource_group_id,
            query_job_config,
            clp_s_query_option,
            output_handle,
        })
    }

    /// Submits the query job to Spider and drives it to completion.
    ///
    /// This method prepares the query tasks' inputs, submits the job, persists the Spider job ID it
    /// was assigned, and then waits for the job to reach a terminal state. On failure, it attempts
    /// to mark the query job as [`QueryJobStatus::Failed`] before the error is returned.
    ///
    /// If no archives are selected, it marks the query job as succeeded without submitting it.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::run_job`]'s return values on failure.
    pub async fn run(self) -> Result<(), Error> {
        tracing::info!(query_job_id = % self.query_job_id, "Starting query job.");

        let result = self.run_job().await;
        if let Err(error) = &result {
            self.report_failure(error).await;
        }
        result
    }

    /// Submits the query job to Spider and waits for it to reach a terminal state.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::plan`]'s return values on failure.
    /// * Forwards [`Self::update_job_status`]'s return values on failure for an empty plan.
    /// * Forwards [`Self::submit`]'s return values on failure.
    /// * Forwards [`Self::to_completion`]'s return values on failure.
    async fn run_job(&self) -> Result<(), Error> {
        let archives_to_search = self.plan().await?;
        if archives_to_search.is_empty() {
            return self
                .update_job_status(
                    Some(QueryJobStatus::Pending),
                    QueryJobStatus::Succeeded,
                    None,
                )
                .await;
        }

        let spider_job_id = self.submit(archives_to_search).await?;
        self.to_completion(spider_job_id).await
    }

    /// Resumes a query job that was already submitted to Spider.
    ///
    /// This method skips submission and waits for the Spider job identified by `spider_job_id` to
    /// reach a terminal state. On failure, it attempts to mark the query job as
    /// [`QueryJobStatus::Failed`] before the error is returned.
    ///
    /// NOTE: It's the caller's responsibility to ensure that the given Spider job ID is associated
    /// with the query job.
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

        let result = self.to_completion(spider_job_id).await;
        if let Err(error) = &result {
            self.report_failure(error).await;
        }
        result
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
    async fn submit(
        &self,
        archives_to_search: Vec<(ArchiveMetadata, ExecutionPolicy)>,
    ) -> Result<SpiderJobId, Error> {
        let num_tasks = archives_to_search.len();
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

    /// Prepares the task inputs for the query job.
    ///
    /// This method retrieves archive metadata from the CLP database, selects the archives matching
    /// the query, and attaches the configured execution policy to each task.
    ///
    /// # Returns
    ///
    /// A vector of tuples on success, where each tuple contains:
    ///
    /// * The [`ArchiveMetadata`] identifying the archive searched by a single query task.
    /// * The [`ExecutionPolicy`] for that task.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`PlanningOption::prepare_task_inputs`]'s return values on failure.
    async fn plan(&self) -> Result<Vec<(ArchiveMetadata, ExecutionPolicy)>, Error> {
        self.context
            .planning_option
            .prepare_task_inputs(
                &self.context.db_pool,
                &self.context.db_config,
                self.query_job_id,
                &self.query_job_config,
            )
            .await
    }

    /// Persists the Spider job ID and marks the query job as running.
    ///
    /// This method associates the given Spider job ID with the query job in the CLP database and
    /// updates the query job status to [`QueryJobStatus::Running`].
    ///
    /// This method also ensures that the job has a valid `dispatch_time`, which the coordinator
    /// uses to mark jobs as dispatched. A coordinator restart may occur before the marker is
    /// persisted, leaving the Spider job running without a valid `dispatch_time`. Therefore, this
    /// method sets the field as part of row update if it has not already been set by the
    /// coordinator.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::QueryJobMetadataCorrupted`] if no pending query job row was updated.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn persist_spider_job_id(
        &self,
        spider_job_id: SpiderJobId,
        num_tasks: i32,
    ) -> Result<(), Error> {
        let query = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `spider_id` = ?, `status` = ?, `num_tasks` = ?, \
             `start_time` = CURRENT_TIMESTAMP(3), `dispatch_time` = COALESCE(`dispatch_time`, \
             CURRENT_TIMESTAMP()) WHERE `id` = ? AND `status` = ?"
        );
        let query = sqlx::query(query)
            .bind(spider_job_id.get())
            .bind(QueryJobStatus::Running)
            .bind(num_tasks)
            .bind(self.query_job_id)
            .bind(QueryJobStatus::Pending);
        let result = query.execute(&self.context.db_pool).await?;
        if 0 == result.rows_affected() {
            return Err(Error::QueryJobMetadataCorrupted(self.query_job_id));
        }
        Ok(())
    }

    /// Waits for the associated Spider job to complete and finalizes the query job.
    ///
    /// This method monitors the specified Spider job until it reaches a terminal state, then
    /// updates the query job according to the Spider job's result.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`QueryJobSubmitter::run_query_job_to_completion`]'s return values on failure.
    /// * Forwards [`Self::update_job_status`]'s return values on failure.
    async fn to_completion(&self, spider_job_id: SpiderJobId) -> Result<(), Error> {
        let outcome = self
            .job_submitter
            .run_query_job_to_completion(spider_job_id, self.context.spider_option.poll_interval)
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
                Some(format!("The Spider job failed: {error_message}")),
            ),
            QueryJobOutcome::Cancelled => (
                QueryJobStatus::Cancelled,
                Some("The Spider job was cancelled.".to_owned()),
            ),
        };
        self.update_job_status(
            Some(QueryJobStatus::Running),
            status,
            status_message.as_deref(),
        )
        .await
    }

    /// Reports a query job failure.
    ///
    /// This method logs the original error and attempts to mark the query job as
    /// [`QueryJobStatus::Failed`] in the CLP database. The stored status message includes the
    /// original error message.
    ///
    /// If updating the job status fails, the status-update error is logged for observability and
    /// otherwise ignored. No update is attempted if the original error already indicates corrupted
    /// metadata.
    async fn report_failure(&self, error: &Error) {
        tracing::error!(
            query_job_id = % self.query_job_id,
            error = % error,
            "Query job failed.",
        );

        let update_error;
        let status_error = if matches!(error, Error::QueryJobMetadataCorrupted(_)) {
            // Skips updating the metadata row since it already cannot be found
            Some(error)
        } else {
            let status_message = format!("Query job failed: {error}");
            update_error = self
                .update_job_status(None, QueryJobStatus::Failed, Some(&status_message))
                .await
                .err();
            update_error.as_ref()
        };

        match status_error {
            None => {}
            Some(status_error @ Error::QueryJobMetadataCorrupted(_)) => {
                tracing::warn!(
                    query_job_id = % self.query_job_id,
                    error = % status_error,
                    "Query job metadata corrupted; skipping update.",
                );
            }
            Some(status_error) => {
                tracing::error!(
                    query_job_id = % self.query_job_id,
                    error = % status_error,
                    "Failed to update job status on a job failure.",
                );
            }
        }
    }

    /// Updates the query job status in the CLP database.
    ///
    /// If `from` is `Some`, only a row whose current status matches it is updated; otherwise the
    /// row is updated regardless of its current status.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::QueryJobMetadataCorrupted`] if no matching query job row can be found.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn update_job_status(
        &self,
        from: Option<QueryJobStatus>,
        to: QueryJobStatus,
        msg: Option<&str>,
    ) -> Result<(), Error> {
        const UPDATE_QUERY: &str = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = ? WHERE `id` = ?"
        );
        const UPDATE_QUERY_WITH_FROM_STATUS: &str = formatcp!("{UPDATE_QUERY} AND `status` = ?");

        let query = sqlx::query(if from.is_some() {
            UPDATE_QUERY_WITH_FROM_STATUS
        } else {
            UPDATE_QUERY
        })
        .bind(to)
        .bind(msg.unwrap_or_default())
        .bind(self.query_job_id);
        let query = match from {
            Some(from) => query.bind(from),
            None => query,
        };

        let result = query.execute(&self.context.db_pool).await?;
        if 0 == result.rows_affected() {
            return Err(Error::QueryJobMetadataCorrupted(self.query_job_id));
        }
        Ok(())
    }
}
