//! The coordinator poll loop that discovers pending CLP query jobs and dispatches them to Spider.
//!
//! The coordinator is responsible for the query jobs in the `query_jobs` table that are in one of
//! the following states:
//!
//! | `status` | `spider_id` | `dispatch_time` | Description                                      |
//! |----------|-------------|-----------------|--------------------------------------------------|
//! | PENDING  | NULL        | NULL            | New jobs awaiting dispatch.                      |
//! | PENDING  | NULL        | NOT NULL        | Jobs dispatched but not yet submitted to Spider. |
//! | RUNNING  | NOT NULL    | NOT NULL        | Jobs submitted to Spider.                        |
//!
//! NOTE:
//!
//! * These are the only legal states for a job that hasn't terminated.
//! * A non-NULL `dispatch_time` indicates that the coordinator has picked up the job and granted it
//!   permission to run under the concurrency limit.

use std::sync::Arc;
use std::time::Duration;

use clp_rust_utils::clp_config::package::config::SearchCoordinator as SearchCoordinatorConfig;
use clp_rust_utils::clp_config::package::config::Spider as SpiderConfig;
use clp_rust_utils::clp_config::package::config::SpiderResourceGroup;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use const_format::formatcp;
use spider_client::SpiderClient;
use spider_core::task::ExecutionPolicy;
use spider_core::task::TimeoutPolicy;
use spider_core::types::id::JobId as SpiderJobId;
use spider_core::types::id::ResourceGroupId;
use tokio::select;
use tokio::sync::Semaphore;
use tokio::time::Instant;
use tokio_util::sync::CancellationToken;
use tonic::transport::Endpoint;

use crate::Error;
use crate::job_handle::QueryJobHandle;
use crate::job_handle::SpiderOption;

/// Coordinator for fetching new query jobs and submitting them to Spider.
pub struct SearchCoordinator {
    resource_group_id: ResourceGroupId,
    spider_client: SpiderClient,
    db_pool: sqlx::MySqlPool,
    spider_option: Arc<SpiderOption>,
    is_first_fetch: bool,
    job_polling_interval: Duration,
    cancellation_token: CancellationToken,
    job_handler_sem: Arc<Semaphore>,
}

impl SearchCoordinator {
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
    /// * The constructed [`SearchCoordinator`].
    /// * The [`CancellationToken`] the caller uses to request shutdown.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::InvalidConfiguration`] if the search coordinator configuration is invalid.
    /// * [`Error::InvalidEndpoint`] if the Spider host and port do not form a valid endpoint.
    /// * Forwards [`SpiderClient::builder`]'s connection return values on failure.
    /// * Forwards [`get_or_create_resource_group_id`]'s return values on failure.
    /// * Forwards [`Self::fetch_submitted_running_jobs`]'s return values on failure.
    pub async fn new(
        coordinator_config: &SearchCoordinatorConfig,
        spider_config: &SpiderConfig,
        db_pool: sqlx::MySqlPool,
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
        let resource_group_id = get_or_create_resource_group_id(
            &coordinator_config.resource_group,
            &spider_client,
            &db_pool,
        )
        .await
        .inspect_err(|e| {
            tracing::error!(error = % e, "Failed to get or create resource group.");
        })?;

        let spider_option = Arc::new(SpiderOption {
            search_task_max_retry: coordinator_config.search_task_max_retry,
            search_task_execution_policy: ExecutionPolicy {
                max_num_instances: 1,
                max_num_retry: coordinator_config.search_task_max_retry,
                timeout_policy: TimeoutPolicy {
                    soft_timeout_ms: coordinator_config.commit_task_soft_timeout_secs.get() * 1000,
                    hard_timeout_ms: coordinator_config.commit_task_hard_timeout_secs.get() * 1000,
                },
            },
            initial_poll_backoff: Duration::from_millis(
                coordinator_config
                    .result_polling
                    .init_backoff_millisecs
                    .get(),
            ),
            max_poll_backoff: Duration::from_millis(
                coordinator_config
                    .result_polling
                    .max_backoff_millisecs
                    .get(),
            ),
        });

        let cancellation_token = CancellationToken::new();

        let coordinator = Self {
            resource_group_id,
            spider_client,
            db_pool,
            spider_option,
            is_first_fetch: true,
            job_polling_interval: Duration::from_millis(
                coordinator_config.job_polling_interval_millisecs.get(),
            ),
            cancellation_token: cancellation_token.clone(),
            job_handler_sem: Arc::new(Semaphore::new(max_concurrent_jobs)),
        };

        // NOTE: The current implementation does not enforce concurrency limits for recovered jobs
        // since they were already submitted to Spider. See #2472.
        for (job_id, spider_job_id) in coordinator.fetch_submitted_running_jobs().await? {
            tracing::info!(
                job_id = % job_id,
                spider_job_id = % spider_job_id,
                "Recovering a previously submitted job."
            );
            let Ok(job_handle) = coordinator.create_job_handle(job_id).await else {
                continue;
            };
            tokio::spawn(async move {
                let _ = job_handle.recover(spider_job_id).await.inspect_err(|e| {
                    tracing::error!(
                        error = % e,
                        job_id = % job_id,
                        spider_job_id = % spider_job_id,
                        "The recovered query job failed."
                    );
                });
            });
        }

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

        tracing::info!("Search coordinator shutting down.");
        Ok(())
    }

    /// Marks the query job identified by `job_id` as [`QueryJobStatus::Failed`].
    ///
    /// This is a best-effort update; if it fails, the error is logged and otherwise ignored.
    async fn mark_job_failed(&self, job_id: QueryJobId, status_msg: &str) {
        const QUERY: &str = formatcp!(
            "UPDATE `{table}` SET `status` = ?, `status_msg` = ?, `update_time` = \
             CURRENT_TIMESTAMP() WHERE `id` = ?;",
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
    /// A job whose config cannot be deserialized is marked [`QueryJobStatus::Failed`] and
    /// skipped; a job whose handle cannot be constructed is skipped as well (and marked
    /// [`QueryJobStatus::Failed`] unless its input config is unsupported, in which case it is
    /// left for the legacy Celery-based compression scheduler).
    ///
    /// # Returns
    ///
    /// The IDs of the fetched jobs that were dispatched in this poll.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::Semaphore`] if acquiring a job handler permit from `job_handler_sem` fails.
    /// * Forwards [`Self::fetch_new_job_rows`]'s return values on failure.
    async fn schedule_new_jobs(&mut self) -> Result<Vec<QueryJobId>, Error> {
        if self.job_handler_sem.available_permits() == 0 {
            return Ok(Vec::new());
        }

        let new_job_rows = self.fetch_new_job_rows().await.inspect_err(|e| {
            tracing::error!(error = % e, "Failed to fetch new jobs from database.");
        })?;

        let dispatched_job_ids: Vec<QueryJobId> = new_job_rows.iter().map(|row| row.id).collect();
        for job_row in new_job_rows {
            let job_id = job_row.id;
            tracing::info!(job_id = % job_id, "Scheduling new job.");
            let Ok(job_handle) = self.create_job_handle(job_id).await else {
                continue;
            };

            let permit = self
                .job_handler_sem
                .clone()
                .acquire_owned()
                .await
                .map_err(|e| {
                    Error::Semaphore(format!("failed to acquire a job handler permit: {e}"))
                })?;

            tokio::spawn(async move {
                let _permit = permit;
                let _ = job_handle.run().await.inspect_err(|e| {
                    tracing::error!(
                        error = % e,
                        job_id = % job_id,
                        "Failed to schedule query job."
                    );
                });
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

    /// Constructs a [`QueryJobHandle`] for the given job.
    ///
    /// A construction failure is logged, and the job is marked [`QueryJobStatus::Failed`] for
    /// any failure other than an unsupported input config, which is only warned and left for
    /// another handler.
    ///
    /// # Returns
    ///
    /// The constructed [`QueryJobHandle`] on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`QueryJobHandle::new`]'s return values on failure.
    async fn create_job_handle(
        &self,
        job_id: QueryJobId,
    ) -> Result<QueryJobHandle<SpiderClient>, Error> {
        let result = QueryJobHandle::new(
            self.db_pool.clone(),
            job_id,
            self.spider_client.clone(),
            self.resource_group_id,
            self.spider_option.clone(),
        );

        if let Err(e) = &result {
            if matches!(e, Error::UnsupportedInputConfig) {
                tracing::warn!(
                    error = % e,
                    job_id = % job_id,
                    "Unsupported input config. Skipping."
                );
            } else {
                tracing::error!(
                    error = % e,
                    job_id = % job_id,
                    "Failed to create query job handle. Skipping."
                );
                self.mark_job_failed(
                    job_id,
                    &format!("Failed to create the query job handle: {e}"),
                )
                .await;
            }
        }

        result
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
            "SELECT `id` FROM `{table}` WHERE `status` = ? AND `dispatch_time` IS NOT NULL ORDER \
             BY `id` ASC;",
            table = QUERY_JOBS_TABLE_NAME,
        );
        const SUBSEQUENT_FETCH_QUERY: &str = formatcp!(
            "SELECT `id` FROM `{table}` WHERE `status` = ? AND `dispatch_time` IS NULL ORDER BY \
             `id` ASC LIMIT ?;",
            table = QUERY_JOBS_TABLE_NAME,
        );

        let query = if self.is_first_fetch {
            self.is_first_fetch = false;
            sqlx::query_as::<_, PendingJobRowProjection>(FIRST_FETCH_QUERY)
                .bind(QueryJobStatus::Pending)
        } else {
            sqlx::query_as::<_, PendingJobRowProjection>(SUBSEQUENT_FETCH_QUERY)
                .bind(QueryJobStatus::Pending)
                .bind(
                    i64::try_from(self.job_handler_sem.available_permits())
                        .expect("limit is bounded by Semaphore::MAX_PERMITS, which fits in i64"),
                )
        };

        let rows = query.fetch_all(&self.db_pool).await?;

        Ok(rows)
    }

    /// Fetches jobs that are still in [`QueryJobStatus::Running`] and were previously submitted by
    /// the search coordinator.
    ///
    /// # Returns
    ///
    /// A vector of tuples on success, each tuple containing:
    ///
    /// * The query job ID.
    /// * The Spider job ID.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    async fn fetch_submitted_running_jobs(&self) -> Result<Vec<(QueryJobId, SpiderJobId)>, Error> {
        const QUERY: &str = formatcp!(
            "SELECT `id`, `spider_id` FROM `{table}` WHERE `status` = ? AND `spider_id` IS NOT \
             NULL;",
            table = QUERY_JOBS_TABLE_NAME,
        );

        let rows = sqlx::query_as::<_, RunningJobRowProjection>(QUERY)
            .bind(QueryJobStatus::Running)
            .fetch_all(&self.db_pool)
            .await?;

        Ok(rows
            .into_iter()
            .map(|row| (row.id, row.spider_job_id))
            .collect())
    }
}

/// A projection of the columns read from a [`QueryJobStatus::Pending`] query job row.
#[derive(Debug, sqlx::FromRow)]
struct PendingJobRowProjection {
    id: QueryJobId,
}

/// A projection of the columns read from a [`QueryJobStatus::Running`] query job row.
#[derive(Debug, sqlx::FromRow)]
struct RunningJobRowProjection {
    id: QueryJobId,
    #[sqlx(rename = "spider_id")]
    spider_job_id: SpiderJobId,
}

/// Retrieves the Spider resource group ID for the configured resource group, registering it if it
/// does not yet exist.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
/// * Forwards [`SpiderClient::add_resource_group`]'s return values on failure.
async fn get_or_create_resource_group_id(
    resource_group_config: &SpiderResourceGroup,
    spider_client: &SpiderClient,
    db_pool: &sqlx::MySqlPool,
) -> Result<ResourceGroupId, Error> {
    const SPIDER_RESOURCE_GROUP_TABLE_NAME: &str = "spider_resource_groups";

    const CREATE_TABLE_QUERY: &str = formatcp!(
        "CREATE TABLE IF NOT EXISTS `{table}` (
            `rg_name` VARCHAR(255) NOT NULL,
            `rg_id` BIGINT UNSIGNED NOT NULL,
            PRIMARY KEY (`rg_name`) USING BTREE
        ) ROW_FORMAT=DYNAMIC",
        table = SPIDER_RESOURCE_GROUP_TABLE_NAME,
    );
    const SELECT_QUERY: &str = formatcp!(
        "SELECT `rg_id` FROM `{table}` WHERE `rg_name` = ?;",
        table = SPIDER_RESOURCE_GROUP_TABLE_NAME,
    );
    const INSERT_QUERY: &str = formatcp!(
        "INSERT INTO `{table}` (`rg_name`, `rg_id`) VALUES (?, ?);",
        table = SPIDER_RESOURCE_GROUP_TABLE_NAME,
    );

    sqlx::query(CREATE_TABLE_QUERY).execute(db_pool).await?;

    let resource_group = resource_group_config.name.as_str();
    let existing_rg_id: Option<u64> = sqlx::query_scalar(SELECT_QUERY)
        .bind(resource_group)
        .fetch_optional(db_pool)
        .await?;
    if let Some(spider_rg_id) = existing_rg_id {
        tracing::info!(
            resource_group = % resource_group,
            spider_rg_id = % spider_rg_id,
            "Resource group already registered. Returning Spider resource group ID."
        );
        return Ok(ResourceGroupId::from(spider_rg_id));
    }

    // NOTE: For now, Spider does not enforce resource group credential validation. The password is
    // hardcoded to be the same as the username.
    let resource_group_id = spider_client
        .add_resource_group(
            resource_group.to_owned(),
            resource_group.as_bytes().to_vec(),
        )
        .await?;

    sqlx::query(INSERT_QUERY)
        .bind(resource_group)
        .bind(resource_group_id.get())
        .execute(db_pool)
        .await
        .inspect_err(|e| {
            tracing::error!(
                error = % e,
                "Failed to insert resource group into database. This might be a race condition. \
                 Restart the service to retry."
            );
        })?;

    Ok(resource_group_id)
}
