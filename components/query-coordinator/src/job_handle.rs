//! Handle for driving a single query job to completion.

use std::cmp::Reverse;
use std::collections::HashSet;
use std::num::NonZeroU32;
use std::num::NonZeroU64;
use std::num::NonZeroUsize;
use std::sync::Arc;
use std::time::Duration;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::dataset::VALID_DATASET_NAME_REGEX;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::SearchJobConfig;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use clp_rust_utils::types::ArchiveId;
use const_format::formatcp;
use non_empty_string::NonEmptyString;
use spider_core::task::ExecutionPolicy;
use spider_core::task::TimeoutPolicy;
use spider_core::types::id::JobId as SpiderJobId;
use spider_core::types::id::ResourceGroupId;
use sqlx::MySql;
use sqlx::MySqlPool;
use sqlx::Transaction;

use crate::Error;
use crate::query_job_submitter::ArchiveMetadata;
use crate::query_job_submitter::QueryJobOutcome;
use crate::query_job_submitter::QueryJobSubmitter;

/// Options for a query job running in Spider.
pub struct SpiderOption {
    pub poll_interval: Duration,
    pub query_task_max_retry: u32,
}

/// Options for selecting the archives to search.
pub struct ArchiveSelectionOptions {
    pub archive_retention_period_millisecs: Option<NonZeroU64>,
    pub max_datasets_per_query: Option<NonZeroUsize>,
}

/// Resources shared by query job handles created by the coordinator.
pub struct QueryJobHandleContext {
    pub db_pool: MySqlPool,
    pub db_config: Database,
    pub archive_selection_options: ArchiveSelectionOptions,
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
    search_job_config: SearchJobConfig,
    clp_s_query_option: ClpSQueryOption,
    output_handle: OutputHandle,
    archive_end_ts_lower_bound_millisecs: Option<i64>,
}

impl<SubmitterType: QueryJobSubmitter> QueryJobHandle<SubmitterType> {
    /// Factory function.
    ///
    /// Only plain search jobs whose results are returned through the results cache can be driven
    /// by this handle; any other job config is rejected so that the caller can fail the job
    /// instead of leaving it unhandled.
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
    /// * [`Error::InvalidQueryJobConfig`] if:
    ///   * The job carries an aggregation config.
    ///   * The job asks for its results to be written to files.
    ///   * The query string is empty.
    ///   * The begin timestamp exceeds the end timestamp.
    pub fn new(
        context: Arc<QueryJobHandleContext>,
        query_job_id: QueryJobId,
        job_submitter: SubmitterType,
        resource_group_id: ResourceGroupId,
        search_job_config: SearchJobConfig,
        output_handle: OutputHandle,
        job_creation_timestamp_millisecs: i64,
    ) -> Result<Self, Error> {
        if search_job_config.aggregation_config.is_some() {
            return Err(Error::InvalidQueryJobConfig(
                "aggregation jobs are not supported".to_owned(),
            ));
        }

        if search_job_config.write_to_file {
            return Err(Error::InvalidQueryJobConfig(
                "writing query results to files is not supported; resubmit the query with its \
                 results buffered in the results cache"
                    .to_owned(),
            ));
        }

        let query_string = NonEmptyString::try_from(search_job_config.query_string.clone())
            .map_err(|_| {
                Error::InvalidQueryJobConfig("query string must not be empty".to_owned())
            })?;

        if let (Some(begin_timestamp), Some(end_timestamp)) = (
            search_job_config.begin_timestamp,
            search_job_config.end_timestamp,
        ) && begin_timestamp > end_timestamp
        {
            return Err(Error::InvalidQueryJobConfig(format!(
                "begin timestamp {begin_timestamp} is greater than end timestamp {end_timestamp}"
            )));
        }

        let archive_end_ts_lower_bound_millisecs = context
            .archive_selection_options
            .archive_retention_period_millisecs
            .map(|period| job_creation_timestamp_millisecs.saturating_sub_unsigned(period.get()));

        let clp_s_query_option = ClpSQueryOption {
            query_string,
            max_num_results: NonZeroU32::new(search_job_config.max_num_results),
            begin_timestamp_millisecs: search_job_config.begin_timestamp,
            end_timestamp_millisecs: search_job_config.end_timestamp,
            ignore_case: search_job_config.ignore_case,
        };

        Ok(Self {
            context,
            query_job_id,
            job_submitter,
            resource_group_id,
            search_job_config,
            clp_s_query_option,
            output_handle,
            archive_end_ts_lower_bound_millisecs,
        })
    }

    /// Submits the query job to Spider and drives it to completion.
    ///
    /// This method prepares the query tasks' inputs, submits the job, persists the Spider job ID it
    /// was assigned, and then waits for the job to reach a terminal state. On failure, it attempts
    /// to persist a terminal status for the query job before the error is returned.
    ///
    /// If no archives are selected, it marks the query job as succeeded without submitting it.
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
            if archives_to_search.is_empty() {
                return self.terminate(QueryJobOutcome::Succeeded).await;
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
    /// * Forwards [`QueryJobSubmitter::submit_query_job`]'s return values on failure.
    /// * Forwards [`Self::start`]'s return values on failure.
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

        self.start(spider_job_id, persisted_num_tasks).await?;
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
        ArchiveSelector {
            context: &self.context,
            search_job_config: &self.search_job_config,
            archive_end_ts_lower_bound_millisecs: self.archive_end_ts_lower_bound_millisecs,
        }
        .select()
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
    /// * [`Error::QueryJobCancelled`] if the query job is in [`QueryJobStatus::Cancelling`].
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
            QueryJobStatus::Cancelling => return Err(Error::QueryJobCancelled),
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
    /// * Forwards [`QueryJobSubmitter::run_query_job_to_completion`]'s return values on failure.
    /// * Forwards [`Self::terminate`]'s return values on failure.
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
            Error::QueryJobCancelled => QueryJobOutcome::Cancelled,
            error => QueryJobOutcome::Failed {
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
    /// * [`QueryJobOutcome::Succeeded`] is permitted from [`QueryJobStatus::Pending`] and
    ///   [`QueryJobStatus::Running`]. From [`QueryJobStatus::Cancelling`], the job is terminated as
    ///   [`QueryJobStatus::Cancelled`] instead, since a cancellation wins over a success.
    /// * [`QueryJobOutcome::Cancelled`] is permitted only from [`QueryJobStatus::Cancelling`].
    /// * [`QueryJobOutcome::Failed`] is permitted from any non-terminal status, since a failure
    ///   wins over both a success and a cancellation.
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
    async fn terminate(&self, outcome: QueryJobOutcome) -> Result<(), Error> {
        const UPDATE_QUERY: &str = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = ? WHERE `id` = ?"
        );
        const CANCELLED_AFTER_COMPLETION_MESSAGE: &str =
            "The query job completed, but it had already been requested to be cancelled.";
        const CANCELLED_MESSAGE: &str = "The query job was cancelled.";

        let mut tx = self.context.db_pool.begin().await?;
        let current_status = self.lock_and_get_status(&mut tx).await?;

        let to = QueryJobStatus::from(&outcome);
        let (status_to_persist, status_message) = match (outcome, current_status) {
            (QueryJobOutcome::Succeeded, QueryJobStatus::Pending | QueryJobStatus::Running) => {
                (to, None)
            }
            (QueryJobOutcome::Cancelled, QueryJobStatus::Cancelling) => {
                (to, Some(CANCELLED_MESSAGE.to_owned()))
            }
            (QueryJobOutcome::Succeeded, QueryJobStatus::Cancelling) => {
                tracing::info!(
                    query_job_id = % self.query_job_id,
                    "{CANCELLED_AFTER_COMPLETION_MESSAGE}",
                );
                (
                    QueryJobStatus::Cancelled,
                    Some(CANCELLED_AFTER_COMPLETION_MESSAGE.to_owned()),
                )
            }
            (QueryJobOutcome::Failed { error_message }, from) if !from.is_terminal() => {
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

impl From<&QueryJobOutcome> for QueryJobStatus {
    fn from(outcome: &QueryJobOutcome) -> Self {
        match outcome {
            QueryJobOutcome::Succeeded => Self::Succeeded,
            QueryJobOutcome::Failed { .. } => Self::Failed,
            QueryJobOutcome::Cancelled => Self::Cancelled,
        }
    }
}

/// Selects the archives a query job should search, and derives each one's task execution policy.
///
/// # Lifetimes
///
/// * `'job_handle_lifetime` - The lifetime of the [`QueryJobHandle`] state borrowed by the
///   selector.
struct ArchiveSelector<'job_handle_lifetime> {
    context: &'job_handle_lifetime QueryJobHandleContext,
    search_job_config: &'job_handle_lifetime SearchJobConfig,
    archive_end_ts_lower_bound_millisecs: Option<i64>,
}

impl ArchiveSelector<'_> {
    /// Prepares the task inputs for the query job.
    ///
    /// This method validates and deduplicates the job's requested datasets, then retrieves from the
    /// CLP database the metadata of the archives matching the query, ordered by descending archive
    /// end timestamp, and attaches an execution policy derived from each archive's size to its
    /// task. Archives that end before the job's archive retention lower bound, if any, are
    /// excluded.
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
    /// * Forwards [`Self::deduplicate_and_validate_requested_datasets`]'s return values on failure.
    /// * Forwards [`Self::ensure_all_required_datasets_exist`]'s return values on failure.
    /// * Forwards [`Self::fetch_archives`]'s return values on failure.
    async fn select(&self) -> Result<Vec<(ArchiveMetadata, ExecutionPolicy)>, Error> {
        let datasets = self.deduplicate_and_validate_requested_datasets()?;
        self.ensure_all_required_datasets_exist(&datasets).await?;

        let mut selected_archives = Vec::new();
        for dataset in &datasets {
            selected_archives.extend(self.fetch_archives(dataset).await?);
        }
        selected_archives.sort_by_key(|archive| Reverse(archive.end_timestamp));

        Ok(selected_archives
            .into_iter()
            .map(|archive| {
                let execution_policy = self.compute_query_task_execution_policy(archive.size);
                (archive, execution_policy)
            })
            .collect())
    }

    /// Validates and deduplicates the datasets requested by a query job.
    ///
    /// # Returns
    ///
    /// The distinct requested datasets on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::InvalidQueryJobConfig`] if:
    ///   * The job doesn't request any datasets, since clp-text queries aren't supported.
    ///   * The job's requested datasets are empty.
    ///   * A dataset name doesn't match [`VALID_DATASET_NAME_REGEX`].
    ///   * The number of distinct datasets exceeds
    ///     [`ArchiveSelectionOptions::max_datasets_per_query`].
    fn deduplicate_and_validate_requested_datasets(
        &self,
    ) -> Result<HashSet<NonEmptyString>, Error> {
        let Some(requested_datasets) = self.search_job_config.datasets.as_deref() else {
            return Err(Error::InvalidQueryJobConfig(
                "clp-text queries are not supported".to_owned(),
            ));
        };

        if requested_datasets.is_empty() {
            return Err(Error::InvalidQueryJobConfig(
                "the datasets list must not be empty".to_owned(),
            ));
        }

        let datasets = requested_datasets
            .iter()
            .map(|dataset| {
                NonEmptyString::new(dataset.clone())
                    .ok()
                    .filter(|name| VALID_DATASET_NAME_REGEX.is_match(name.as_str()))
                    .ok_or_else(|| {
                        Error::InvalidQueryJobConfig(format!("invalid dataset name `{dataset}`"))
                    })
            })
            .collect::<Result<HashSet<_>, _>>()?;

        if let Some(max_datasets_per_query) = self
            .context
            .archive_selection_options
            .max_datasets_per_query
            && datasets.len() > max_datasets_per_query.get()
        {
            return Err(Error::InvalidQueryJobConfig(format!(
                "the number of requested datasets ({}) exceeds `max_datasets_per_query` \
                 ({max_datasets_per_query})",
                datasets.len()
            )));
        }

        Ok(datasets)
    }

    /// Checks that every requested dataset exists in the metadata database.
    ///
    /// `datasets` is assumed to be non-empty, since an empty set would make the generated `IN`
    /// clause invalid SQL. [`Self::deduplicate_and_validate_requested_datasets`] guarantees this by
    /// rejecting an empty request.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::InvalidQueryJobConfig`] if any requested dataset doesn't exist.
    /// * Forwards [`sqlx::query::QueryScalar::fetch_one`]'s return values on failure.
    async fn ensure_all_required_datasets_exist(
        &self,
        datasets: &HashSet<NonEmptyString>,
    ) -> Result<(), Error> {
        let datasets_table = self.context.db_config.datasets_table_name();
        let mut query_builder = sqlx::QueryBuilder::<sqlx::MySql>::new(format!(
            "SELECT COUNT(*) FROM `{datasets_table}` WHERE `name` IN ("
        ));
        let mut separated_datasets = query_builder.separated(", ");
        for dataset in datasets {
            separated_datasets.push_bind(dataset.as_str());
        }
        query_builder.push(")");

        let num_existing_datasets: i64 = query_builder
            .build_query_scalar()
            .fetch_one(&self.context.db_pool)
            .await?;
        if usize::try_from(num_existing_datasets).ok() != Some(datasets.len()) {
            return Err(Error::InvalidQueryJobConfig(
                "one or more requested datasets don't exist".to_owned(),
            ));
        }

        Ok(())
    }

    /// Selects archives from one dataset that overlap the query's time range and retention window.
    ///
    /// # Returns
    ///
    /// The selected archives on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    async fn fetch_archives(
        &self,
        dataset: &NonEmptyString,
    ) -> Result<Vec<ArchiveMetadata>, Error> {
        let archives_table = self
            .context
            .db_config
            .archives_table_name(Some(dataset.as_str()));
        let mut query_builder = sqlx::QueryBuilder::<sqlx::MySql>::new(format!(
            "SELECT `id`, `size`, `end_timestamp` FROM `{archives_table}` WHERE TRUE"
        ));
        if let Some(end_timestamp) = self.search_job_config.end_timestamp {
            query_builder
                .push(" AND `begin_timestamp` <= ")
                .push_bind(end_timestamp);
        }
        if let Some(begin_timestamp) = self.search_job_config.begin_timestamp {
            query_builder
                .push(" AND `end_timestamp` >= ")
                .push_bind(begin_timestamp);
        }
        if let Some(lower_bound) = self.archive_end_ts_lower_bound_millisecs {
            query_builder
                .push(" AND (`end_timestamp` >= ")
                .push_bind(lower_bound)
                .push(" OR `end_timestamp` = 0)");
        }

        Ok(query_builder
            .build_query_as::<ArchiveRowProjection>()
            .fetch_all(&self.context.db_pool)
            .await?
            .into_iter()
            .map(|row| ArchiveMetadata {
                id: row.id,
                dataset: Some(dataset.clone()),
                size: row.size,
                end_timestamp: row.end_timestamp,
            })
            .collect())
    }

    /// Derives the execution policy of the query task that searches an archive of `archive_size`
    /// bytes.
    ///
    /// The soft timeout is an aggressive estimate of the task's execution time, whereas the hard
    /// timeout is a conservative deadline that assumes the entire archive is extracted.
    ///
    /// # Returns
    ///
    /// The query task's execution policy.
    fn compute_query_task_execution_policy(&self, archive_size: u64) -> ExecutionPolicy {
        const BYTES_PER_MIB: u64 = 1024 * 1024;
        const SOFT_TIMEOUT_MILLISECS_PER_MIB: u64 = 10;
        const HARD_TIMEOUT_MILLISECS_PER_MIB: u64 = 2 * 1000;

        /// The minimum timeout accepted by Spider, in milliseconds.
        const MIN_TIMEOUT_MILLISECS: u64 = 100;

        /// The maximum timeout accepted by Spider, in milliseconds (24 hours).
        const MAX_TIMEOUT_MILLISECS: u64 = 1000 * 60 * 60 * 24;

        let hard_timeout_ms = (archive_size.saturating_mul(HARD_TIMEOUT_MILLISECS_PER_MIB)
            / BYTES_PER_MIB)
            .clamp(MIN_TIMEOUT_MILLISECS + 1, MAX_TIMEOUT_MILLISECS);
        let soft_timeout_ms = (archive_size.saturating_mul(SOFT_TIMEOUT_MILLISECS_PER_MIB)
            / BYTES_PER_MIB)
            .clamp(MIN_TIMEOUT_MILLISECS, hard_timeout_ms - 1);

        ExecutionPolicy {
            max_num_retry: self.context.spider_option.query_task_max_retry,
            timeout_policy: TimeoutPolicy {
                soft_timeout_ms,
                hard_timeout_ms,
            },
            ..ExecutionPolicy::default()
        }
    }
}

/// Columns projected from an archives table.
///
/// [`ArchiveMetadata`] can't be decoded directly from a row since the dataset is encoded in the
/// archives table's name rather than stored in a column.
#[derive(sqlx::FromRow)]
struct ArchiveRowProjection {
    id: ArchiveId,
    #[sqlx(try_from = "i64")]
    size: u64,
    end_timestamp: i64,
}
