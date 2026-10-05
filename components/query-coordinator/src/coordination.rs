//! The coordinator poll loop that discovers pending CLP query jobs and dispatches them to Spider.
//!
//! The coordinator is responsible for the query jobs in the `query_jobs` table that are in one of
//! the following states:
//!
//! | `status`   | `spider_id` | `dispatch_time` | Description                                   |
//! |------------|-------------|-----------------|-----------------------------------------------|
//! | PENDING    | NULL        | NULL            | New jobs awaiting dispatch.                   |
//! | PENDING    | NULL        | NOT NULL        | Jobs dispatched, not yet submitted to Spider. |
//! | RUNNING    | NOT NULL    | NOT NULL        | Jobs submitted to Spider.                     |
//! | CANCELLING | ANY         | ANY             | Jobs the API server has requested to cancel.  |
//!
//! NOTE:
//!
//! * These are the only legal states for a job that hasn't terminated.
//! * A non-NULL `dispatch_time` indicates that the coordinator has picked up the job and granted it
//!   permission to run under the concurrency limit.
//! * CANCELLING is non-terminal and is reachable from both PENDING and RUNNING.
//!
//! TODO: Handle CANCELLING jobs in the coordinator: a background coroutine that cancels the Spider
//! job of any job sitting in CANCELLING, and a restart sweep that drives CANCELLING jobs to
//! CANCELLED.

use std::num::NonZeroU64;
use std::sync::Arc;
use std::time::Duration;

use clp_rust_utils::clp_config::package::config::ArchiveOutput as ArchiveOutputConfig;
use clp_rust_utils::clp_config::package::config::Database as DatabaseConfig;
use clp_rust_utils::clp_config::package::config::QueryCoordinator as CoordinatorConfig;
use clp_rust_utils::clp_config::package::config::ResultsCache as ResultsCacheConfig;
use clp_rust_utils::clp_config::package::config::Spider as SpiderConfig;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::QueryJobType;
use clp_rust_utils::job_config::SearchJobConfig;
use clp_rust_utils::task_io::query::OutputHandle;
use const_format::formatcp;
use spider_client::SpiderClient;
use spider_core::types::id::JobId as SpiderJobId;
use spider_core::types::id::ResourceGroupId;
use spider_core::types::resource_group::ExternalResourceGroupCredentials;
use tokio::select;
use tokio::sync::Semaphore;
use tokio::time::Instant;
use tokio_util::sync::CancellationToken;
use tonic::transport::Endpoint;

use crate::Error;
use crate::job_handle::ArchiveSelectionOptions;
use crate::job_handle::QueryJobHandle;
use crate::job_handle::QueryJobHandleContext;
use crate::job_handle::SpiderOption;

/// Coordinator for fetching new query jobs and submitting them to Spider.
pub struct Coordinator {
    resource_group_id: ResourceGroupId,
    spider_client: SpiderClient,
    db_pool: sqlx::MySqlPool,
    job_handle_context: Arc<QueryJobHandleContext>,
    output_handle: OutputHandle,
    is_first_fetch: bool,
    job_polling_interval: Duration,
    cancellation_token: CancellationToken,
    job_handler_permits: Arc<Semaphore>,
}

impl Coordinator {
    /// Factory function.
    ///
    /// On construction, this recovers query jobs that a previous coordinator instance had already
    /// submitted to Spider (those still [`QueryJobStatus::Running`] with a Spider job ID) by
    /// spawning a detached handle to drive each one to completion.
    ///
    /// # Returns
    ///
    /// A tuple on success, containing:
    ///
    /// * The constructed [`Coordinator`].
    /// * The [`CancellationToken`] the caller uses to request shutdown.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::InvalidConfiguration`] if the query coordinator configuration is invalid.
    /// * [`Error::InvalidEndpoint`] if the Spider host and port do not form a valid endpoint.
    /// * Forwards [`SpiderClient::builder`]'s connection return values on failure.
    /// * Forwards [`ExternalResourceGroupCredentials::from_env`]'s return values on failure.
    /// * Forwards [`SpiderClient::add_or_verify_resource_group`]'s return values on failure.
    /// * Forwards [`Self::recover_submitted_jobs`]'s return values on failure.
    pub async fn new(
        coordinator_config: &CoordinatorConfig,
        spider_config: &SpiderConfig,
        results_cache_config: &ResultsCacheConfig,
        archive_output_config: &ArchiveOutputConfig,
        db_pool: sqlx::MySqlPool,
        db_config: DatabaseConfig,
    ) -> Result<(Self, CancellationToken), Error> {
        let max_concurrent_jobs = coordinator_config.max_concurrent_jobs.get();
        if max_concurrent_jobs > Semaphore::MAX_PERMITS {
            return Err(Error::InvalidConfiguration(format!(
                "`max_concurrent_jobs` must not exceed {}, got {max_concurrent_jobs}",
                Semaphore::MAX_PERMITS,
            )));
        }

        let spider_host = spider_config.host.as_str();
        let spider_port = spider_config.port;
        let endpoint_str = format!("http://{spider_host}:{spider_port}");
        let endpoint = Endpoint::from_shared(endpoint_str)
            .inspect_err(|e| {
                tracing::error!(error = % e, "Failed to create Spider endpoint.");
            })
            .map_err(|e| Error::InvalidEndpoint(e.to_string()))?;
        let spider_client = SpiderClient::builder(endpoint)
            .connect()
            .await
            .inspect_err(|e| {
                tracing::error!(error = % e, "Failed to connect to Spider.");
            })?;
        let resource_group_id = spider_client
            .add_or_verify_resource_group(ExternalResourceGroupCredentials::from_env()?)
            .await
            .inspect_err(|e| {
                tracing::error!(error = % e, "Failed to add or verify resource group.");
            })?;

        let job_handle_context = Arc::new(QueryJobHandleContext {
            db_pool: db_pool.clone(),
            db_config,
            archive_selection_options: ArchiveSelectionOptions {
                archive_retention_period_millisecs: archive_output_config
                    .retention_period
                    .map(|period| period.saturating_mul(MILLISECS_PER_MINUTE)),
                max_datasets_per_query: coordinator_config.max_datasets_per_query,
            },
            spider_option: SpiderOption {
                poll_interval: Duration::from_millis(
                    coordinator_config.result_polling_interval_millisecs.get(),
                ),
                query_task_max_retry: coordinator_config.query_task_max_retry,
            },
        });

        let cancellation_token = CancellationToken::new();

        let coordinator = Self {
            resource_group_id,
            spider_client,
            db_pool,
            job_handle_context,
            output_handle: OutputHandle::ResultsCache {
                uri: results_cache_config.uri(),
            },
            is_first_fetch: true,
            job_polling_interval: Duration::from_millis(
                coordinator_config.job_polling_interval_millisecs.get(),
            ),
            cancellation_token: cancellation_token.clone(),
            job_handler_permits: Arc::new(Semaphore::new(max_concurrent_jobs)),
        };

        coordinator.recover_submitted_jobs().await?;

        Ok((coordinator, cancellation_token))
    }

    /// Runs the coordinator's poll loop until cancelled.
    ///
    /// On each iteration, this method fetches the pending query jobs, spawns a detached handle to
    /// drive each one, and then sleeps until the next poll or until the cancellation token is
    /// triggered. The jobs dispatched in the iteration are marked once the sleep elapses, so their
    /// update does not contend with concurrent job submissions during the poll interval.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::schedule_new_jobs`]'s return values on failure.
    /// * Forwards [`Self::mark_jobs_dispatched`]'s return values on failure.
    pub async fn run(mut self) -> Result<(), Error> {
        let cancellation_token = self.cancellation_token.clone();
        loop {
            let now = Instant::now();

            let dispatched_job_ids;
            select! {
                () = cancellation_token.cancelled() => {
                    break;
                }
                result = self.schedule_new_jobs() => {
                    dispatched_job_ids = result.inspect_err(|e| {
                        tracing::error!(error = % e, "Failed to schedule new jobs.");
                    })?;
                }
            }

            let elapsed = now.elapsed();
            let sleep_duration = self.job_polling_interval.saturating_sub(elapsed);
            if sleep_duration.is_zero() {
                tokio::task::yield_now().await;
            } else if tokio::time::timeout(sleep_duration, cancellation_token.cancelled())
                .await
                .is_ok()
            {
                break;
            }

            self.mark_jobs_dispatched(&dispatched_job_ids).await?;
        }

        tracing::info!("Coordinator shutting down.");
        Ok(())
    }

    /// Spawns a detached handle to drive each query job that a previous coordinator instance had
    /// already submitted to Spider.
    ///
    /// A job whose handle cannot be constructed is marked [`QueryJobStatus::Failed`] and skipped,
    /// since nothing else would ever drive it to a terminal status.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::fetch_submitted_running_jobs`]'s return values on failure.
    async fn recover_submitted_jobs(&self) -> Result<(), Error> {
        // NOTE: The current implementation does not enforce concurrency limits for recovered jobs
        // since they were already submitted to Spider. See #2472.
        for SubmittedJob {
            id: job_id,
            spider_job_id,
            search_job_config,
            job_creation_timestamp_millisecs,
        } in self.fetch_submitted_running_jobs().await?
        {
            tracing::info!(
                job_id = % job_id,
                spider_job_id = % spider_job_id,
                "Recovering a previously submitted job."
            );
            let job_handle = match QueryJobHandle::new(
                self.job_handle_context.clone(),
                job_id,
                self.spider_client.clone(),
                self.resource_group_id,
                search_job_config,
                self.output_handle.clone(),
                job_creation_timestamp_millisecs,
            ) {
                Ok(job_handle) => job_handle,
                Err(e) => {
                    tracing::error!(
                        error = % e,
                        job_id = % job_id,
                        "Failed to create the query job handle for recovery. Skipping."
                    );
                    self.mark_job_failed(
                        job_id,
                        &format!("Failed to create the query job handle for recovery: {e}"),
                    )
                    .await;
                    continue;
                }
            };
            tokio::spawn(async move {
                // `QueryJobHandle::recover` already logs and persists its own failures.
                let _ = job_handle.recover(spider_job_id).await;
            });
        }

        Ok(())
    }

    /// Marks the query job identified by `job_id` as [`QueryJobStatus::Failed`].
    ///
    /// This is a best-effort update; if it fails, the error is logged and otherwise ignored.
    async fn mark_job_failed(&self, job_id: QueryJobId, status_msg: &str) {
        const QUERY: &str = formatcp!(
            "UPDATE `{table}` SET `status` = ?, `status_msg` = ? WHERE `id` = ?;",
            table = QUERY_JOBS_TABLE_NAME,
        );
        tracing::info!(job_id = % job_id, "Failing the query job.");
        if let Err(e) = sqlx::query(QUERY)
            .bind(QueryJobStatus::Failed)
            .bind(status_msg)
            .bind(job_id)
            .execute(&self.db_pool)
            .await
        {
            tracing::error!(
                error = % e,
                job_id = % job_id,
                "Failed to mark the query job as failed."
            );
        }
    }

    /// Fetches pending query jobs and spawns a detached handle to drive each one as permitted by
    /// the job-handler semaphore.
    ///
    /// A job whose config cannot be deserialized is marked [`QueryJobStatus::Failed`] and skipped;
    /// a job whose handle cannot be constructed, which includes every job config the coordinator
    /// doesn't support, is marked failed and skipped as well.
    ///
    /// # Returns
    ///
    /// The IDs of all the query jobs fetched in this poll, including those that were skipped, so
    /// that a skipped job isn't re-fetched on every subsequent poll.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::fetch_new_job_rows`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if `job_handler_permits` has been closed, which the coordinator never does.
    async fn schedule_new_jobs(&mut self) -> Result<Vec<QueryJobId>, Error> {
        if self.job_handler_permits.available_permits() == 0 {
            return Ok(Vec::new());
        }

        let new_job_rows = self.fetch_new_job_rows().await.inspect_err(|e| {
            tracing::error!(error = % e, "Failed to fetch new jobs from database.");
        })?;

        let dispatched_job_ids: Vec<QueryJobId> = new_job_rows.iter().map(|row| row.id).collect();
        for job_row in new_job_rows {
            let job_id = job_row.id;
            let search_job_config: SearchJobConfig =
                match rmp_serde::from_slice(&job_row.serialized_search_job_config) {
                    Ok(search_job_config) => search_job_config,
                    Err(e) => {
                        tracing::error!(
                            error = % e,
                            job_id = % job_id,
                            "Failed to deserialize search job config. Skipping."
                        );
                        self.mark_job_failed(
                            job_id,
                            &format!("Failed to deserialize search job config: {e}"),
                        )
                        .await;
                        continue;
                    }
                };
            tracing::info!(job_id = % job_id, "Scheduling new job.");
            let job_handle = match QueryJobHandle::new(
                self.job_handle_context.clone(),
                job_id,
                self.spider_client.clone(),
                self.resource_group_id,
                search_job_config,
                self.output_handle.clone(),
                job_row.job_creation_timestamp_millisecs,
            ) {
                Ok(job_handle) => job_handle,
                Err(e) => {
                    tracing::error!(
                        error = % e,
                        job_id = % job_id,
                        "Failed to create the query job handle. Skipping."
                    );
                    self.mark_job_failed(
                        job_id,
                        &format!("Failed to create the query job handle: {e}"),
                    )
                    .await;
                    continue;
                }
            };

            let permit = self
                .job_handler_permits
                .clone()
                .acquire_owned()
                .await
                .expect("the job handler semaphore is never closed");

            tokio::spawn(async move {
                let _permit = permit;
                // `QueryJobHandle::run` already logs and persists its own failures.
                let _ = job_handle.run().await;
            });
        }
        Ok(dispatched_job_ids)
    }

    /// Marks the query jobs identified by `job_ids` with the current dispatch time.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    async fn mark_jobs_dispatched(&self, job_ids: &[QueryJobId]) -> Result<(), Error> {
        if job_ids.is_empty() {
            return Ok(());
        }

        let mut tx = self.db_pool.begin().await?;
        for chunk in job_ids.chunks(1000) {
            let mut query_builder = sqlx::QueryBuilder::<sqlx::MySql>::new(formatcp!(
                "UPDATE `{table}` SET `dispatch_time` = COALESCE(`dispatch_time`, \
                 CURRENT_TIMESTAMP()) WHERE `id` IN (",
                table = QUERY_JOBS_TABLE_NAME,
            ));
            let mut separated_ids = query_builder.separated(", ");
            for job_id in chunk {
                separated_ids.push_bind(job_id);
            }
            query_builder.push(");");
            query_builder.build().execute(&mut *tx).await?;
        }
        tx.commit().await?;

        Ok(())
    }

    /// Fetches pending query jobs eligible for dispatch.
    ///
    /// The first fetch after startup returns every [`QueryJobStatus::Pending`] job whose
    /// `dispatch_time` is set, so that jobs dispatched but not started by the previous coordinator
    /// instance can be re-dispatched. No explicit limit is imposed because:
    ///
    /// * This query runs only once, so limiting it could leave previously dispatched jobs
    ///   unfetched.
    /// * The recovery set is bounded by the previous coordinator's concurrency limit.
    ///
    /// Every subsequent fetch returns only [`QueryJobStatus::Pending`] jobs whose dispatch time is
    /// not set. The available permit count determines how many rows are fetched, ensuring that the
    /// coordinator does not fetch more jobs than it can dispatch during the current polling
    /// iteration.
    ///
    /// # Returns
    ///
    /// A vector of rows projected from the query job table on success, each row represents a
    /// pending query job.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    async fn fetch_new_job_rows(&mut self) -> Result<Vec<PendingJobRowProjection>, Error> {
        const FIRST_FETCH_QUERY: &str = formatcp!(
            "SELECT `id`, `job_config`, CAST(UNIX_TIMESTAMP(`creation_time`) * 1000 AS SIGNED) AS \
             `job_creation_timestamp_millisecs` FROM `{table}` WHERE `type` = ? AND `status` = ? \
             AND `dispatch_time` IS NOT NULL ORDER BY `id` ASC;",
            table = QUERY_JOBS_TABLE_NAME,
        );
        const SUBSEQUENT_FETCH_QUERY: &str = formatcp!(
            "SELECT `id`, `job_config`, CAST(UNIX_TIMESTAMP(`creation_time`) * 1000 AS SIGNED) AS \
             `job_creation_timestamp_millisecs` FROM `{table}` WHERE `type` = ? AND `status` = ? \
             AND `dispatch_time` IS NULL ORDER BY `id` ASC LIMIT ?;",
            table = QUERY_JOBS_TABLE_NAME,
        );

        let query = if self.is_first_fetch {
            self.is_first_fetch = false;
            sqlx::query_as::<_, PendingJobRowProjection>(FIRST_FETCH_QUERY)
                .bind(QueryJobType::SearchOrAggregation)
                .bind(QueryJobStatus::Pending)
        } else {
            sqlx::query_as::<_, PendingJobRowProjection>(SUBSEQUENT_FETCH_QUERY)
                .bind(QueryJobType::SearchOrAggregation)
                .bind(QueryJobStatus::Pending)
                .bind(
                    i64::try_from(self.job_handler_permits.available_permits())
                        .expect("limit is bounded by Semaphore::MAX_PERMITS, which fits in i64"),
                )
        };

        let rows = query.fetch_all(&self.db_pool).await?;

        Ok(rows)
    }

    /// Fetches jobs that are still in [`QueryJobStatus::Running`] and were previously submitted by
    /// the query coordinator.
    ///
    /// A running job whose config cannot be deserialized is marked [`QueryJobStatus::Failed`] and
    /// skipped.
    ///
    /// # Returns
    ///
    /// A vector of [`SubmittedJob`]s on success, each represents a running query job to recover.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    async fn fetch_submitted_running_jobs(&self) -> Result<Vec<SubmittedJob>, Error> {
        const QUERY: &str = formatcp!(
            "SELECT `id`, `spider_id`, `job_config`, CAST(UNIX_TIMESTAMP(`creation_time`) * 1000 \
             AS SIGNED) AS `job_creation_timestamp_millisecs` FROM `{table}` WHERE `type` = ? AND \
             `status` = ? AND `spider_id` IS NOT NULL;",
            table = QUERY_JOBS_TABLE_NAME,
        );

        let mut recovery_context = Vec::new();
        for row in sqlx::query_as::<_, RunningJobRowProjection>(QUERY)
            .bind(QueryJobType::SearchOrAggregation)
            .bind(QueryJobStatus::Running)
            .fetch_all(&self.db_pool)
            .await?
        {
            let search_job_config: SearchJobConfig = match rmp_serde::from_slice(
                &row.serialized_search_job_config,
            ) {
                Ok(search_job_config) => search_job_config,
                Err(e) => {
                    tracing::error!(
                        error = % e,
                        job_id = % row.id,
                        "Failed to deserialize search job config of a running job. The database \
                         might be corrupted. Skipping."
                    );
                    self.mark_job_failed(
                        row.id,
                        &format!("Failed to deserialize search job config: {e}"),
                    )
                    .await;
                    continue;
                }
            };
            recovery_context.push(SubmittedJob {
                id: row.id,
                spider_job_id: row.spider_job_id,
                search_job_config,
                job_creation_timestamp_millisecs: row.job_creation_timestamp_millisecs,
            });
        }

        Ok(recovery_context)
    }
}

const MILLISECS_PER_MINUTE: NonZeroU64 =
    NonZeroU64::new(60_000).expect("constant should not be zero");

/// A query job that was submitted to Spider by a previous coordinator instance.
struct SubmittedJob {
    id: QueryJobId,
    spider_job_id: SpiderJobId,
    search_job_config: SearchJobConfig,
    job_creation_timestamp_millisecs: i64,
}

/// A projection of the columns read from a [`QueryJobStatus::Pending`] query job row.
#[derive(Debug, sqlx::FromRow)]
struct PendingJobRowProjection {
    id: QueryJobId,
    #[sqlx(rename = "job_config")]
    serialized_search_job_config: Vec<u8>,
    job_creation_timestamp_millisecs: i64,
}

/// A projection of the columns read from a [`QueryJobStatus::Running`] query job row.
#[derive(Debug, sqlx::FromRow)]
struct RunningJobRowProjection {
    id: QueryJobId,
    #[sqlx(rename = "spider_id")]
    spider_job_id: SpiderJobId,
    #[sqlx(rename = "job_config")]
    serialized_search_job_config: Vec<u8>,
    job_creation_timestamp_millisecs: i64,
}
