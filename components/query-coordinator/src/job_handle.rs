//! Lifecycle management for one coordinator-planned query job.

use std::sync::Arc;
use std::time::Duration;

use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use non_empty_string::NonEmptyString;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId as SpiderJobId;
use spider_core::types::id::ResourceGroupId;
use sqlx::MySqlPool;

use crate::Error;
use crate::query_job_submitter::QueryJobOutcome;
use crate::query_job_submitter::QueryJobSubmitter;

/// The coordinator-prepared inputs for one Spider query graph.
pub struct QueryPlan {
    /// Query behavior shared by every archive task.
    pub clp_s_query_option: ClpSQueryOption,

    /// Result destination shared by every archive task.
    pub output_handle: OutputHandle,

    /// One optional dataset and non-empty archive ID pair per task.
    pub archives: Vec<(Option<NonEmptyString>, NonEmptyString)>,

    /// Execution policy applied to every archive task.
    pub query_task_execution_policy: ExecutionPolicy,
}

/// Spider polling options shared by query-job handles.
pub struct SpiderOption {
    /// Delay before the first Spider job-state poll.
    pub initial_poll_backoff: Duration,

    /// Maximum delay between Spider job-state polls.
    pub max_poll_backoff: Duration,
}

/// Drives one already-planned query job through submission and terminal persistence.
pub struct QueryJobHandle<SubmitterType: QueryJobSubmitter> {
    db_pool: MySqlPool,
    query_job_id: QueryJobId,
    job_submitter: SubmitterType,
    resource_group_id: ResourceGroupId,
    query_plan: QueryPlan,
    spider_option: Arc<SpiderOption>,
}

impl<SubmitterType: QueryJobSubmitter> QueryJobHandle<SubmitterType> {
    /// Constructs a handle for an already-planned query job.
    pub fn new(
        db_pool: MySqlPool,
        query_job_id: QueryJobId,
        job_submitter: SubmitterType,
        resource_group_id: ResourceGroupId,
        query_plan: QueryPlan,
        spider_option: Arc<SpiderOption>,
    ) -> Self {
        Self {
            db_pool,
            query_job_id,
            job_submitter,
            resource_group_id,
            query_plan,
            spider_option,
        }
    }

    /// Submits the prepared graph and drives the query job to a terminal state.
    ///
    /// On an orchestration failure, this method makes a best-effort attempt to mark the CLP query
    /// job as failed before returning the original error.
    ///
    /// # Errors
    ///
    /// Returns an error if submission, submission persistence, polling, or terminal persistence
    /// fails.
    pub async fn run(self) -> Result<(), Error> {
        tracing::info!(query_job_id = %self.query_job_id, "Starting query job.");

        let result = self.submit_and_wait().await;
        if let Err(error) = &result {
            self.report_failure(error).await;
        }
        result
    }

    /// Resumes a query job that was already submitted to Spider.
    ///
    /// The caller must ensure `spider_job_id` belongs to this CLP query job.
    ///
    /// # Errors
    ///
    /// Returns an error if polling or terminal persistence fails.
    pub async fn recover(self, spider_job_id: SpiderJobId) -> Result<(), Error> {
        tracing::info!(
            query_job_id = %self.query_job_id,
            spider_job_id = %spider_job_id,
            "Recovering query job.",
        );

        let result = self.to_completion(spider_job_id).await;
        if let Err(error) = &result {
            self.report_failure(error).await;
        }
        result
    }

    async fn submit_and_wait(&self) -> Result<(), Error> {
        let num_tasks = self.query_plan.archives.len();
        let persisted_num_tasks =
            i32::try_from(num_tasks).map_err(|_| Error::TooManyQueryTasks(num_tasks))?;
        let spider_job_id = self
            .job_submitter
            .submit_query_job(
                self.query_job_id,
                self.resource_group_id,
                self.query_plan.clp_s_query_option.clone(),
                self.query_plan.output_handle.clone(),
                self.query_plan.archives.clone(),
                self.query_plan.query_task_execution_policy.clone(),
            )
            .await?;

        tracing::info!(
            query_job_id = %self.query_job_id,
            spider_job_id = %spider_job_id,
            num_tasks,
            "Query job submitted.",
        );

        self.persist_submission(spider_job_id, persisted_num_tasks)
            .await?;
        self.to_completion(spider_job_id).await
    }

    async fn persist_submission(
        &self,
        spider_job_id: SpiderJobId,
        num_tasks: i32,
    ) -> Result<(), Error> {
        let query = format!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `spider_id` = ?, `status` = ?, `num_tasks` = ?, \
             `start_time` = CURRENT_TIMESTAMP(3) WHERE `id` = ? AND `status` = ?"
        );
        let result = sqlx::query(&query)
            .bind(spider_job_id.get())
            .bind(i32::from(QueryJobStatus::Running))
            .bind(num_tasks)
            .bind(self.query_job_id)
            .bind(i32::from(QueryJobStatus::Pending))
            .execute(&self.db_pool)
            .await?;

        if 1 != result.rows_affected() {
            return Err(Error::JobNotPending(self.query_job_id));
        }
        Ok(())
    }

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
            query_job_id = %self.query_job_id,
            spider_job_id = %spider_job_id,
            outcome = ?outcome,
            "Query job reached a terminal Spider state.",
        );

        match outcome {
            QueryJobOutcome::Succeeded => {
                self.update_terminal_status(QueryJobStatus::Succeeded, "", false)
                    .await
            }
            QueryJobOutcome::Failed { error_message } => {
                self.update_terminal_status(
                    QueryJobStatus::Failed,
                    &format!("The Spider query job failed: {error_message}"),
                    false,
                )
                .await
            }
            QueryJobOutcome::UnexpectedlyCancelled => {
                self.update_terminal_status(
                    QueryJobStatus::Failed,
                    "The Spider query job was unexpectedly cancelled.",
                    false,
                )
                .await
            }
        }
    }

    async fn report_failure(&self, error: &Error) {
        tracing::error!(
            query_job_id = %self.query_job_id,
            error = %error,
            "Query-job orchestration failed.",
        );

        if let Err(status_error) = self
            .update_terminal_status(
                QueryJobStatus::Failed,
                &format!("Query-job orchestration failed: {error}"),
                true,
            )
            .await
        {
            tracing::error!(
                query_job_id = %self.query_job_id,
                error = %status_error,
                "Failed to persist the query-job failure.",
            );
        }
    }

    /// Updates a non-terminal query job while preserving every existing terminal or cancellation
    /// state. When `allow_pending` is false, only a running job may transition.
    async fn update_terminal_status(
        &self,
        status: QueryJobStatus,
        status_message: &str,
        allow_pending: bool,
    ) -> Result<(), Error> {
        let eligible_statuses = if allow_pending { "?, ?" } else { "?" };
        let query = format!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = LEFT(?, 512), \
             `duration` = \
             CASE WHEN `start_time` IS NULL THEN 0 ELSE TIMESTAMPDIFF(MICROSECOND, `start_time`, \
             CURRENT_TIMESTAMP(3)) / 1000000.0 END WHERE `id` = ? AND `status` IN \
             ({eligible_statuses})"
        );
        let mut query = sqlx::query(&query)
            .bind(i32::from(status))
            .bind(status_message)
            .bind(self.query_job_id)
            .bind(i32::from(QueryJobStatus::Running));
        if allow_pending {
            query = query.bind(i32::from(QueryJobStatus::Pending));
        }
        query.execute(&self.db_pool).await?;
        Ok(())
    }
}
