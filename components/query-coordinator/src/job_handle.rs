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
    pub output_handle: OutputHandle,
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
    pub fn new(
        context: Arc<QueryJobHandleContext>,
        query_job_id: QueryJobId,
        job_submitter: SubmitterType,
        resource_group_id: ResourceGroupId,
        query_job_config: SearchJobConfig,
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
        })
    }

    /// Submits the prepared graph and drives the query job to a terminal state.
    ///
    /// On a submission failure, this method makes a best-effort attempt to mark the CLP query job
    /// as failed before returning the original error. After the job is durably running, monitoring
    /// and terminal-persistence failures leave it running so recovery can reattach to Spider.
    ///
    /// If no matching row is found when persisting the Spider ID, the job may have been cancelled,
    /// deleted, or claimed by another coordinator job handler. Anyhow, this handle no longer owns
    /// it, so it skips trying to report a job failure.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::plan_and_submit`]'s return values on failure.
    /// * Forwards [`Self::to_completion`]'s return values on failure.
    pub async fn run(self) -> Result<(), Error> {
        tracing::info!(query_job_id = % self.query_job_id, "Starting query job.");

        match self.plan_and_submit().await {
            Ok(Some(spider_job_id)) => self.to_completion(spider_job_id).await,
            Ok(None) => Ok(()),
            Err(error) => {
                if !matches!(error, Error::SqlxNoRowsAffected(_)) {
                    self.report_failure(&error).await;
                }
                Err(error)
            }
        }
    }

    /// Plans the query inputs and submits the query job, or marks it as succeeded if no archives
    /// are selected.
    ///
    /// # Returns
    ///
    /// On success, the submitted Spider job ID, or `None` if no archives are selected.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::prepare_task_inputs`]'s return values on failure.
    /// * [`Error::SqlxNoRowsAffected`] if no pending row is updated for an empty plan.
    /// * Forwards [`execute_update`]'s return values on failure for an empty plan.
    /// * Forwards [`Self::submit`]'s return values on failure.
    async fn plan_and_submit(&self) -> Result<Option<SpiderJobId>, Error> {
        let archives_to_query = self.prepare_task_inputs().await?;
        if archives_to_query.is_empty() {
            let query = sqlx::query(formatcp!(
                "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = '', \
                 `num_tasks` = 0, `start_time` = CURRENT_TIMESTAMP(3), `duration` = 0 WHERE `id` \
                 = ? AND `status` = ?"
            ))
            .bind(QueryJobStatus::Succeeded)
            .bind(self.query_job_id)
            .bind(QueryJobStatus::Pending);
            if !execute_update(query, &self.context.db_pool).await? {
                return Err(Error::SqlxNoRowsAffected(format!(
                    "no pending query job row found for query job {}",
                    self.query_job_id
                )));
            }
            return Ok(None);
        }
        self.submit(archives_to_query).await.map(Some)
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
    async fn submit(
        &self,
        archives_to_query: Vec<(ArchiveMetadata, ExecutionPolicy)>,
    ) -> Result<SpiderJobId, Error> {
        let num_tasks = archives_to_query.len();
        let persisted_num_tasks =
            i32::try_from(num_tasks).map_err(|_| Error::TooManyQueryTasks(num_tasks))?;
        let spider_job_id = self
            .job_submitter
            .submit_query_job(
                self.query_job_id,
                self.resource_group_id,
                self.clp_s_query_option.clone(),
                self.context.output_handle.clone(),
                archives_to_query,
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
    /// Returns an error if:
    ///
    /// * Forwards [`PlanningOption::prepare_task_inputs`]'s return values on failure.
    async fn prepare_task_inputs(&self) -> Result<Vec<(ArchiveMetadata, ExecutionPolicy)>, Error> {
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
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::SqlxNoRowsAffected`] if no pending query job row was updated.
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
        let query = sqlx::query(query)
            .bind(spider_job_id.get())
            .bind(QueryJobStatus::Running)
            .bind(num_tasks)
            .bind(self.query_job_id)
            .bind(QueryJobStatus::Pending);
        if !execute_update(query, &self.context.db_pool).await? {
            return Err(Error::SqlxNoRowsAffected(format!(
                "no pending query job row found for query job {} (Spider job ID {})",
                self.query_job_id, spider_job_id
            )));
        }
        Ok(())
    }

    /// Starts the Spider job if needed, waits for completion, and finalizes the query job.
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
                Some(format!("The Spider query job failed: {error_message}")),
            ),
            QueryJobOutcome::Cancelled => (
                QueryJobStatus::Cancelled,
                Some("The Spider query job was cancelled.".to_owned()),
            ),
        };
        if !self
            .update_job_status(QueryJobStatus::Running, status, status_message.as_deref())
            .await?
        {
            return Err(Error::SqlxNoRowsAffected(format!(
                "no running query job row found for query job {}",
                self.query_job_id
            )));
        }
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
                QueryJobStatus::Pending,
                QueryJobStatus::Failed,
                Some(&format!("Query job orchestration failed: {error}")),
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

    /// Updates the query job status in the CLP database.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn update_job_status(
        &self,
        from: QueryJobStatus,
        to: QueryJobStatus,
        msg: Option<&str>,
    ) -> Result<bool, sqlx::Error> {
        let query = sqlx::query(formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = ? WHERE `id` = ? \
             AND `status` = ?"
        ))
        .bind(to)
        .bind(msg.unwrap_or_default())
        .bind(self.query_job_id)
        .bind(from);
        execute_update(query, &self.context.db_pool).await
    }
}

/// Executes an SQL update and reports whether any row was affected.
async fn execute_update(
    query: sqlx::query::Query<'_, sqlx::MySql, sqlx::mysql::MySqlArguments>,
    db_pool: &MySqlPool,
) -> Result<bool, sqlx::Error> {
    let result = query.execute(db_pool).await?;
    Ok(result.rows_affected() > 0)
}
