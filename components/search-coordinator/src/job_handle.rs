//! Handle for driving a single query job to completion.
//!
//! The job-lifecycle and CLP-DB status functions (`run`, `recover`, `report_failure`,
//! `persist_spider_job_id`, `get_job_status`, `update_job_status`) are ported from the compression
//! coordinator's `job_handle.rs`. Submission, task-input preparation, and completion-handling
//! (`new`, `submit_and_wait`, `to_completion`) are not yet implemented and currently stubbed.

use std::sync::Arc;
use std::time::Duration;

use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use const_format::formatcp;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId as SpiderJobId;
use spider_core::types::id::ResourceGroupId;
use sqlx::MySqlPool;

use crate::Error;
use crate::search_job_submitter::QueryJobOutcome;
use crate::search_job_submitter::QueryJobSubmitter;

/// Options for a query job running in Spider.
pub struct SpiderOption {
    pub search_task_max_retry: u32,
    pub search_task_execution_policy: ExecutionPolicy,
    pub initial_poll_backoff: Duration,
    pub max_poll_backoff: Duration,
}

/// Handles the asynchronous submission of a query job and the retrieval of its result.
///
/// # Type Parameters
///
/// * `SubmitterType` - The type of the job submitter for Spider job submission.
#[allow(dead_code)]
pub struct QueryJobHandle<SubmitterType: QueryJobSubmitter> {
    db_pool: MySqlPool,
    query_job_id: QueryJobId,
    job_submitter: SubmitterType,
    resource_group_id: ResourceGroupId,

    dataset: Option<String>,

    spider_option: Arc<SpiderOption>,
}

impl<SubmitterType: QueryJobSubmitter> QueryJobHandle<SubmitterType> {
    /// Factory function.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * The query job's input config is invalid (validation not yet implemented).
    pub const fn new(
        db_pool: MySqlPool,
        query_job_id: QueryJobId,
        job_submitter: SubmitterType,
        resource_group_id: ResourceGroupId,
        spider_option: Arc<SpiderOption>,
    ) -> Result<Self, Error> {
        Ok(Self {
            db_pool,
            query_job_id,
            job_submitter,
            resource_group_id,
            dataset: None,
            spider_option,
        })
    }

    /// Submits the query job to Spider and drives it to completion.
    ///
    /// This method prepares the search tasks' inputs, submits the job, persists the Spider job ID
    /// it was assigned, and then waits for the job to reach a terminal state. On any failure, the
    /// query job is marked as [`QueryJobStatus::Failed`] before the error is returned.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::submit_and_wait`]'s return values on failure.
    pub async fn run(self) -> Result<(), Error> {
        tracing::info!(query_job_id = % self.query_job_id, "Starting query job.");

        let result = self.submit_and_wait().await;
        if let Err(err) = &result {
            self.report_failure(err).await;
        }

        result
    }

    /// Resumes a query job that was already submitted to Spider.
    ///
    /// This method skips submission and waits for the Spider job identified by `spider_job_id` to
    /// reach a terminal state. On failure, the query job is marked as [`QueryJobStatus::Failed`]
    /// before the error is returned.
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
        match self.to_completion(spider_job_id).await {
            Ok(()) => Ok(()),
            Err(err) => {
                self.report_failure(&err).await;
                Err(err)
            }
        }
    }

    /// Submits the query job to Spider and waits for it to reach a terminal state.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`QueryJobSubmitter::submit_query_job`]'s return values on failure.
    /// * Forwards [`Self::persist_spider_job_id`]'s return values on failure.
    /// * Forwards [`Self::to_completion`]'s return values on failure.
    async fn submit_and_wait(&self) -> Result<(), Error> {
        let spider_job_id = self
            .job_submitter
            .submit_query_job(
                self.query_job_id,
                self.resource_group_id,
                self.dataset.clone(),
            )
            .await?;
        tracing::info!(
            query_job_id = % self.query_job_id,
            spider_job_id = % spider_job_id,
            "Query job submitted.",
        );

        // NOTE: The search task graph is not yet designed, so the task count is a placeholder
        // until the per-archive search task inputs are constructed.
        const NUM_TASKS: usize = 1;
        self.persist_spider_job_id(spider_job_id, NUM_TASKS).await?;
        tracing::info!(
            query_job_id = % self.query_job_id,
            spider_job_id = % spider_job_id,
            "Query job submission persisted.",
        );

        self.to_completion(spider_job_id).await
    }

    /// Reports a query job failure.
    ///
    /// This method logs the original error and attempts to mark the query job as
    /// [`QueryJobStatus::Failed`] in the CLP database. The stored status message includes the
    /// original error message.
    ///
    /// If updating the job status fails, the status-update error is logged for observability and
    /// otherwise ignored.
    async fn report_failure(&self, err: &Error) {
        tracing::error!(
            query_job_id = % self.query_job_id,
            error = % err,
            "Query job failed.",
        );
        let status_message = format!("Query job failed: {err}");
        if let Err(e) = self
            .update_job_status(QueryJobStatus::Failed, Some(status_message))
            .await
        {
            tracing::error!(
                query_job_id = % self.query_job_id,
                error = % e,
                "Failed to update job status on a job failure.",
            );
        }
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
    /// * [`Error::TooManySearchTasks`] if `num_tasks` exceeds `i32`'s range.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn persist_spider_job_id(
        &self,
        spider_job_id: SpiderJobId,
        num_tasks: usize,
    ) -> Result<(), Error> {
        let num_tasks =
            i32::try_from(num_tasks).map_err(|_| Error::TooManySearchTasks(num_tasks))?;
        sqlx::query(formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `spider_id` = ?, `status` = ?, `num_tasks` = ?, \
             `start_time` = CURRENT_TIMESTAMP(3), `dispatch_time` = COALESCE(`dispatch_time`, \
             CURRENT_TIMESTAMP()) WHERE `id` = ?"
        ))
        .bind(spider_job_id.get())
        .bind(QueryJobStatus::Running)
        .bind(num_tasks)
        .bind(self.query_job_id)
        .execute(&self.db_pool)
        .await?;

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
            "Query job reached a terminal state.",
        );

        match outcome {
            QueryJobOutcome::Succeeded => Ok(()),
            QueryJobOutcome::Failed { error_message } => {
                self.update_job_status(
                    QueryJobStatus::Failed,
                    Some(format!("The Spider query job failed: {error_message}")),
                )
                .await
            }
            QueryJobOutcome::Cancelled => {
                self.update_job_status(
                    QueryJobStatus::Killed,
                    Some("The Spider query job was cancelled.".to_owned()),
                )
                .await
            }
        }
    }

    /// Reads the current status of the query job from the CLP database.
    ///
    /// # Returns
    ///
    /// The current [`QueryJobStatus`] of the query job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryScalar::fetch_one`]'s return values on failure.
    #[allow(dead_code)]
    async fn get_job_status(&self) -> Result<QueryJobStatus, Error> {
        let job_status: QueryJobStatus = sqlx::query_scalar(formatcp!(
            "SELECT `status` FROM `{QUERY_JOBS_TABLE_NAME}` WHERE `id` = ?"
        ))
        .bind(self.query_job_id)
        .fetch_one(&self.db_pool)
        .await?;

        Ok(job_status)
    }

    /// Updates the query job status in the CLP database.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn update_job_status(
        &self,
        job_status: QueryJobStatus,
        status_message: Option<String>,
    ) -> Result<(), Error> {
        let status_message = status_message.as_ref().map_or("", String::as_str);
        sqlx::query(formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = ? WHERE `id` = ?"
        ))
        .bind(job_status)
        .bind(status_message)
        .bind(self.query_job_id)
        .execute(&self.db_pool)
        .await?;

        Ok(())
    }
}
