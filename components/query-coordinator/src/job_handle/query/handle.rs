//! Handle for driving a single query job to completion.

use std::sync::Arc;

use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobConfig;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use const_format::formatcp;
use spider_core::task::ExecutionPolicy;
use spider_core::types::id::JobId as SpiderJobId;
use spider_core::types::id::ResourceGroupId;
use sqlx::MySql;
use sqlx::Transaction;

use super::archive_selector::ArchiveSelector;
use super::task_graph::build_query_task_graph;
use crate::Error;
use crate::job_handle::ArchiveMetadata;
use crate::job_handle::QueryJobHandleContext;
use crate::spider_job_submitter::JobOutcome;
use crate::spider_job_submitter::SpiderJobSubmitter;

/// Handles the asynchronous submission of a query job and the retrieval of its result.
///
/// # Type Parameters
///
/// * `SubmitterType` - The type of the job submitter for Spider job submission.
pub struct QueryJobHandle<SubmitterType: SpiderJobSubmitter> {
    context: Arc<QueryJobHandleContext>,
    query_job_id: QueryJobId,
    job_submitter: SubmitterType,
    resource_group_id: ResourceGroupId,
    query_job_config: QueryJobConfig,
    job_creation_timestamp_millisecs: i64,
}

impl<SubmitterType: SpiderJobSubmitter> QueryJobHandle<SubmitterType> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created [`QueryJobHandle`] for the given query job.
    #[must_use]
    pub const fn new(
        context: Arc<QueryJobHandleContext>,
        query_job_id: QueryJobId,
        job_submitter: SubmitterType,
        resource_group_id: ResourceGroupId,
        query_job_config: QueryJobConfig,
        job_creation_timestamp_millisecs: i64,
    ) -> Self {
        Self {
            context,
            query_job_id,
            job_submitter,
            resource_group_id,
            query_job_config,
            job_creation_timestamp_millisecs,
        }
    }

    /// Submits the query job to Spider and drives it to completion.
    ///
    /// This method builds the task graph and its inputs, submits the job, persists the assigned
    /// Spider job ID, and then waits for the job to reach a terminal state. On failure, it attempts
    /// to persist a terminal status for the query job before the error is returned.
    ///
    /// If no archives are selected and no timeline is requested, it marks the query job as
    /// succeeded without submitting it.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::plan`]'s return values on failure.
    /// * Forwards [`Self::terminate`]'s return values on failure for an empty plan.
    /// * Forwards [`Self::submit`]'s return values on failure.
    /// * Forwards [`Self::to_completion`]'s return values on failure.
    pub async fn run(self) -> Result<(), Error> {
        tracing::info!(query_job_id = % self.query_job_id, "Starting query job.");

        let result = async {
            let archives_to_search = self.plan().await?;
            if archives_to_search.is_empty() && !self.query_job_config.output.has_timeline() {
                return self.terminate(JobOutcome::Succeeded).await;
            }

            let spider_job_id = self.submit(archives_to_search).await?;
            self.to_completion(spider_job_id).await
        }
        .await;
        if let Err(error) = &result {
            self.finalize_on_error(error).await;
        }
        result
    }

    /// Resumes a query job that was already submitted to Spider.
    ///
    /// This method skips submission and waits for the Spider job identified by `spider_job_id` to
    /// reach a terminal state. On failure, it attempts to persist a terminal status for the query
    /// job before the error is returned.
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
            self.finalize_on_error(error).await;
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
    /// * Forwards [`build_query_task_graph`]'s return values on failure.
    /// * Forwards [`SpiderJobSubmitter::submit_job`]'s return values on failure.
    /// * Forwards [`Self::start`]'s return values on failure.
    async fn submit(
        &self,
        archives_to_search: Vec<(ArchiveMetadata, ExecutionPolicy)>,
    ) -> Result<SpiderJobId, Error> {
        let num_tasks = archives_to_search.len();
        let persisted_num_tasks =
            i32::try_from(num_tasks).map_err(|_| Error::TooManyQueryTasks(num_tasks))?;
        let spider_job_id = {
            let (graph, input) = build_query_task_graph(
                self.query_job_id,
                &self.query_job_config.query,
                &self.query_job_config.output,
                archives_to_search,
            )?;
            let spider_job_id = self
                .job_submitter
                .submit_job(self.resource_group_id, &graph, &input)
                .await?;

            tracing::info!(
                query_job_id = % self.query_job_id,
                spider_job_id = % spider_job_id,
                num_tasks = graph.get_num_tasks(),
                "Submitted query job to Spider.",
            );
            spider_job_id
        };

        tracing::info!(
            query_job_id = % self.query_job_id,
            spider_job_id = % spider_job_id,
            num_tasks,
            "Query job submitted.",
        );

        if let Err(start_error) = self.start(spider_job_id, persisted_num_tasks).await {
            let _ = self
                .job_submitter
                .cancel_job(spider_job_id)
                .await
                .inspect_err(|e| {
                    tracing::warn!(
                        query_job_id = % self.query_job_id,
                        spider_job_id = % spider_job_id,
                        error = % e,
                        "Failed to cancel the Spider job after failing to start the query job.",
                    );
                });
            return Err(start_error);
        }
        Ok(spider_job_id)
    }

    /// Prepares the task inputs for the query job.
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
    /// * Forwards [`ArchiveSelector::select`]'s return values on failure.
    async fn plan(&self) -> Result<Vec<(ArchiveMetadata, ExecutionPolicy)>, Error> {
        ArchiveSelector::new(
            &self.context.db_pool,
            &self.context.db_config,
            &self.context.archive_selection_options,
        )
        .select(
            &self.query_job_config.query,
            self.job_creation_timestamp_millisecs,
            self.context.spider_option.query_task_max_retry,
        )
        .await
    }

    /// Persists the Spider job ID and marks the query job as running.
    ///
    /// This method associates the given Spider job ID with the query job in the CLP database and
    /// updates the query job status from [`QueryJobStatus::Pending`] to
    /// [`QueryJobStatus::Running`], in a transaction that locks the query job's row.
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
    /// * [`Error::QueryJobCancelled`] if the query job is in [`QueryJobStatus::CancelRequested`].
    /// * [`Error::InvalidQueryJobStatusTransition`] if the query job is in any other status than
    ///   [`QueryJobStatus::Pending`].
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`Self::lock_and_get_status`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    async fn start(&self, spider_job_id: SpiderJobId, num_tasks: i32) -> Result<(), Error> {
        const UPDATE_QUERY: &str = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `spider_id` = ?, `status` = ?, `num_tasks` = ?, \
             `start_time` = CURRENT_TIMESTAMP(3), `dispatch_time` = COALESCE(`dispatch_time`, \
             CURRENT_TIMESTAMP()) WHERE `id` = ?"
        );

        let mut tx = self.context.db_pool.begin().await?;
        match self.lock_and_get_status(&mut tx).await? {
            QueryJobStatus::Pending => {}
            QueryJobStatus::CancelRequested => return Err(Error::QueryJobCancelled),
            from => {
                return Err(Error::InvalidQueryJobStatusTransition {
                    from,
                    to: QueryJobStatus::Running,
                });
            }
        }

        sqlx::query(UPDATE_QUERY)
            .bind(spider_job_id.get())
            .bind(QueryJobStatus::Running)
            .bind(num_tasks)
            .bind(self.query_job_id)
            .execute(&mut *tx)
            .await?;
        tx.commit().await?;

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
    /// * Forwards [`SpiderJobSubmitter::run_job_to_completion`]'s return values on failure.
    /// * Forwards [`Self::terminate`]'s return values on failure.
    async fn to_completion(&self, spider_job_id: SpiderJobId) -> Result<(), Error> {
        let outcome = self
            .job_submitter
            .run_job_to_completion(spider_job_id, self.context.spider_option.poll_interval)
            .await?;

        tracing::info!(
            query_job_id = % self.query_job_id,
            spider_job_id = % spider_job_id,
            outcome = ? outcome,
            "Query job reached a terminal Spider state.",
        );

        self.terminate(outcome).await
    }

    /// Finalizes the query job after the handle failed to drive it to completion.
    ///
    /// This method logs `error` as a job-handle failure and then attempts to terminate the query
    /// job with the terminal status implied by `error`. Nothing is persisted if `error` shows that
    /// the job's row is gone or that the job already reached a terminal status through another
    /// path.
    ///
    /// A failed attempt is logged and otherwise ignored, so that it can't re-enter this path.
    async fn finalize_on_error(&self, error: &Error) {
        tracing::error!(
            query_job_id = % self.query_job_id,
            error = % error,
            "Query job failed.",
        );

        let outcome = match error {
            Error::QueryJobMetadataCorrupted(_) => return,
            Error::InvalidQueryJobStatusTransition { from, .. } if from.is_terminal() => return,
            Error::QueryJobCancelled => JobOutcome::Cancelled,
            error => JobOutcome::Failed {
                error_message: format!("Query job failed: {error}"),
            },
        };

        let Err(terminate_error) = self.terminate(outcome).await else {
            return;
        };
        match terminate_error {
            Error::InvalidQueryJobStatusTransition { from, .. } if from.is_terminal() => {
                tracing::warn!(
                    query_job_id = % self.query_job_id,
                    from = ? from,
                    "Query job already reached a terminal status; skipping the status update.",
                );
            }
            terminate_error => {
                tracing::error!(
                    query_job_id = % self.query_job_id,
                    error = % terminate_error,
                    "Failed to update job status on a job failure.",
                );
            }
        }
    }

    /// Terminates the query job with the given outcome in the CLP database.
    ///
    /// This method runs in a transaction that locks the query job's row, and resolves the status to
    /// persist from the locked status as follows:
    ///
    /// * [`JobOutcome::Succeeded`] is permitted from [`QueryJobStatus::Pending`] and
    ///   [`QueryJobStatus::Running`]. From [`QueryJobStatus::CancelRequested`] and
    ///   [`QueryJobStatus::Cancelling`], the job is terminated as [`QueryJobStatus::Cancelled`]
    ///   instead, since a cancellation wins over a success.
    /// * [`JobOutcome::Cancelled`] is permitted only from [`QueryJobStatus::CancelRequested`] and
    ///   [`QueryJobStatus::Cancelling`]. From [`QueryJobStatus::Running`], the job is terminated as
    ///   [`QueryJobStatus::Failed`] instead.
    /// * [`JobOutcome::Failed`] is permitted from any non-terminal status, since a failure wins
    ///   over both a success and a cancellation.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::InvalidQueryJobStatusTransition`] if the query job's current status doesn't
    ///   permit a transition to `outcome`'s status.
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`Self::lock_and_get_status`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    async fn terminate(&self, outcome: JobOutcome) -> Result<(), Error> {
        const UPDATE_QUERY: &str = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = ? WHERE `id` = ?"
        );
        const CANCELLED_AFTER_COMPLETION_MESSAGE: &str =
            "The query job completed, but it had already been requested to be cancelled.";
        const CANCELLED_MESSAGE: &str = "The query job was cancelled.";
        const CANCELLED_UNEXPECTED_MESSAGE: &str =
            "The query job was cancelled unexpectedly without a cancellation request.";

        let mut tx = self.context.db_pool.begin().await?;
        let current_status = self.lock_and_get_status(&mut tx).await?;

        let to = QueryJobStatus::from(&outcome);
        let (status_to_persist, status_message) = match (outcome, current_status) {
            (JobOutcome::Succeeded, QueryJobStatus::Pending | QueryJobStatus::Running) => {
                (to, None)
            }
            (
                JobOutcome::Cancelled,
                QueryJobStatus::CancelRequested | QueryJobStatus::Cancelling,
            ) => (to, Some(CANCELLED_MESSAGE.to_owned())),
            (
                JobOutcome::Succeeded,
                QueryJobStatus::CancelRequested | QueryJobStatus::Cancelling,
            ) => {
                tracing::info!(
                    query_job_id = % self.query_job_id,
                    "{CANCELLED_AFTER_COMPLETION_MESSAGE}",
                );
                (
                    QueryJobStatus::Cancelled,
                    Some(CANCELLED_AFTER_COMPLETION_MESSAGE.to_owned()),
                )
            }
            (JobOutcome::Cancelled, QueryJobStatus::Running) => {
                tracing::error!(
                    query_job_id = % self.query_job_id,
                    "{CANCELLED_UNEXPECTED_MESSAGE}",
                );
                (
                    QueryJobStatus::Failed,
                    Some(CANCELLED_UNEXPECTED_MESSAGE.to_owned()),
                )
            }
            (JobOutcome::Failed { error_message }, from) if !from.is_terminal() => {
                (to, Some(error_message))
            }
            (_, from) => return Err(Error::InvalidQueryJobStatusTransition { from, to }),
        };

        sqlx::query(UPDATE_QUERY)
            .bind(status_to_persist)
            .bind(status_message)
            .bind(self.query_job_id)
            .execute(&mut *tx)
            .await?;
        tx.commit().await?;

        Ok(())
    }

    /// Reads the query job's current status, locking its row until `tx` ends.
    ///
    /// # Returns
    ///
    /// The query job's current status on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::QueryJobMetadataCorrupted`] if the query job's row no longer exists.
    /// * Forwards [`sqlx::query::QueryScalar::fetch_optional`]'s return values on failure.
    async fn lock_and_get_status(
        &self,
        tx: &mut Transaction<'_, MySql>,
    ) -> Result<QueryJobStatus, Error> {
        const SELECT_QUERY: &str =
            formatcp!("SELECT `status` FROM `{QUERY_JOBS_TABLE_NAME}` WHERE `id` = ? FOR UPDATE");

        sqlx::query_scalar::<_, QueryJobStatus>(SELECT_QUERY)
            .bind(self.query_job_id)
            .fetch_optional(&mut **tx)
            .await?
            .ok_or(Error::QueryJobMetadataCorrupted(self.query_job_id))
    }
}

impl From<&JobOutcome> for QueryJobStatus {
    fn from(outcome: &JobOutcome) -> Self {
        match outcome {
            JobOutcome::Succeeded => Self::Succeeded,
            JobOutcome::Failed { .. } => Self::Failed,
            JobOutcome::Cancelled => Self::Cancelled,
        }
    }
}

#[cfg(test)]
mod tests {
    use std::convert::identity;
    use std::sync::Arc;
    use std::time::Duration;

    use async_trait::async_trait;
    use clp_rust_utils::clp_config::package::config::Database;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::dataset::NonEmptyDatasetNames;
    use clp_rust_utils::job_config::QueryJobConfig;
    use clp_rust_utils::job_config::QueryJobId;
    use clp_rust_utils::job_config::QueryOutput;
    use clp_rust_utils::job_config::ResultsCacheOutput;
    use clp_rust_utils::job_config::SearchQuery;
    use clp_rust_utils::job_config::TimeRange;
    use clp_rust_utils::types::ArchiveId;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;
    use spider_client::error::ClientError;
    use spider_core::task::ExecutionPolicy;
    use spider_core::task::TaskGraph;
    use spider_core::types::id::JobId as SpiderJobId;
    use spider_core::types::id::ResourceGroupId;
    use spider_core::types::io::TaskGraphInput;
    use sqlx::MySqlPool;

    use super::QueryJobHandle;
    use crate::Error;
    use crate::job_handle::ArchiveMetadata;
    use crate::job_handle::ArchiveSelectionOptions;
    use crate::job_handle::QueryJobHandleContext;
    use crate::job_handle::SpiderOption;
    use crate::spider_job_submitter::JobOutcome;
    use crate::spider_job_submitter::SpiderJobSubmitter;

    const QUERY_JOB_ID: QueryJobId = 42;
    const RESOURCE_GROUP_ID: ResourceGroupId = ResourceGroupId::from(7);
    const TRANSPORT_ERROR_MESSAGE: &str = "connection lost during job registration";

    #[derive(Clone)]
    struct CheckingSubmitter {
        config: QueryJobConfig,
        dataset: DatasetName,
        archive_id: ArchiveId,
        policy: ExecutionPolicy,
    }

    #[async_trait]
    impl SpiderJobSubmitter for CheckingSubmitter {
        async fn submit_job(
            &self,
            resource_group_id: ResourceGroupId,
            graph: &TaskGraph,
            input: &TaskGraphInput,
        ) -> Result<SpiderJobId, Error> {
            assert_eq!(resource_group_id, RESOURCE_GROUP_ID);
            assert_eq!(graph.get_num_tasks(), 1);
            let task = graph.get_task(0).expect("the search task should exist");
            assert_eq!(task.get_tdl_context().package, "clp");
            assert_eq!(task.get_tdl_context().task_func, "query::clp_s_search");
            assert_eq!(task.get_execution_policy(), &self.policy);

            let payloads = input.clone().into_positional_inputs(identity);
            let [query_job_id, query, dataset, archive_id, output] = payloads.as_slice() else {
                panic!(
                    "expected five positional search task inputs, got {}",
                    payloads.len()
                );
            };
            assert_eq!(
                rmp_serde::from_slice::<QueryJobId>(query_job_id).expect("decode query job ID"),
                QUERY_JOB_ID
            );
            assert_eq!(
                rmp_serde::from_slice::<SearchQuery>(query).expect("decode search query"),
                self.config.query
            );
            assert_eq!(
                rmp_serde::from_slice::<DatasetName>(dataset).expect("decode dataset name"),
                self.dataset
            );
            assert_eq!(
                rmp_serde::from_slice::<ArchiveId>(archive_id).expect("decode archive ID"),
                self.archive_id
            );
            assert_eq!(
                rmp_serde::from_slice::<QueryOutput>(output).expect("decode query output"),
                self.config.output
            );

            Err(ClientError::Transport(TRANSPORT_ERROR_MESSAGE.to_owned()).into())
        }

        async fn run_job_to_completion(
            &self,
            _spider_job_id: SpiderJobId,
            _poll_interval: Duration,
        ) -> Result<JobOutcome, Error> {
            panic!("failed registration must not start a Spider job");
        }

        async fn cancel_job(&self, _spider_job_id: SpiderJobId) -> Result<(), Error> {
            panic!("failed registration leaves no Spider job to cancel");
        }
    }

    #[tokio::test]
    async fn submit_passes_search_graph_and_inputs_and_forwards_registration_error() {
        let archive = ArchiveMetadata {
            id: "00000000-0000-0000-0000-000000000001"
                .parse()
                .expect("the fixture archive ID is a valid UUID"),
            dataset: DatasetName::default(),
            uncompressed_size: 1,
            end_timestamp: 0,
        };
        let config = QueryJobConfig {
            query: SearchQuery {
                datasets: NonEmptyDatasetNames::try_from(vec![archive.dataset.clone()])
                    .expect("the fixture query searches one dataset"),
                query_string: NonEmptyString::from_static_str("level: error"),
                time_range: TimeRange::default(),
                ignore_case: false,
            },
            output: QueryOutput::ResultsCache(ResultsCacheOutput {
                max_num_results: None,
                aggregation: None,
            }),
        };
        let policy = ExecutionPolicy {
            max_num_retry: 4,
            ..ExecutionPolicy::default()
        };
        let submitter = CheckingSubmitter {
            config: config.clone(),
            dataset: archive.dataset.clone(),
            archive_id: archive.id,
            policy: policy.clone(),
        };

        let db_pool = MySqlPool::connect_lazy("mysql://localhost/clp-db")
            .expect("the fixture database URL is valid");
        db_pool.close().await;
        let handle = QueryJobHandle::new(
            Arc::new(QueryJobHandleContext {
                db_pool,
                db_config: Database::default(),
                archive_selection_options: ArchiveSelectionOptions {
                    archive_retention_period_millisecs: None,
                    max_datasets_per_query: None,
                },
                spider_option: SpiderOption {
                    poll_interval: Duration::from_secs(1),
                    query_task_max_retry: policy.max_num_retry,
                },
            }),
            QUERY_JOB_ID,
            submitter,
            RESOURCE_GROUP_ID,
            config,
            0,
        );

        let error = handle
            .submit(vec![(archive, policy)])
            .await
            .expect_err("Spider registration should fail");
        let Error::SpiderClient(ClientError::Transport(message)) = error else {
            panic!("expected Spider transport error, got {error:?}");
        };
        assert_eq!(message, TRANSPORT_ERROR_MESSAGE);
    }
}
