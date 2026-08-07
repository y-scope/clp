//! The coordinator poll loop that discovers pending CLP compression jobs and dispatches them to
//! Spider.

use std::sync::Arc;
use std::time::Duration;

use clp_rust_utils::clp_config::package::config::CompressionCoordinator as CoordinatorConfig;
use clp_rust_utils::clp_config::package::config::Database as DatabaseConfig;
use clp_rust_utils::clp_config::package::config::Spider as SpiderConfig;
use clp_rust_utils::clp_config::package::config::SpiderResourceGroup;
use clp_rust_utils::job_config::ClpIoConfig;
use clp_rust_utils::job_config::CompressionJobId;
use clp_rust_utils::job_config::CompressionJobStatus;
use clp_rust_utils::serde::BrotliMsgpack;
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
use crate::job_handle::S3CompressionJobHandle;
use crate::job_handle::SpiderOption;

/// Coordinator for fetching new compression jobs and submitting them to Spider.
pub struct Coordinator {
    resource_group_id: ResourceGroupId,
    spider_client: SpiderClient,
    db_pool: sqlx::MySqlPool,
    db_config: DatabaseConfig,
    spider_option: Arc<SpiderOption>,
    job_polling_interval: Duration,
    cancellation_token: CancellationToken,
    job_handler_sem: Arc<Semaphore>,
}

impl Coordinator {
    /// Factory function.
    ///
    /// On construction, this begins recovering all compression jobs that a previous coordinator
    /// instance had already submitted to Spider (those still [`CompressionJobStatus::Running`] with
    /// a Spider job ID). During the restart phase, no concurrency limit is imposed, so all
    /// recovered jobs are resumed immediately.
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
    /// * [`Error::InvalidConfiguration`] if the compression coordinator configuration is invalid.
    /// * [`Error::InvalidEndpoint`] if the Spider host and port do not form a valid endpoint.
    /// * Forwards [`SpiderClient::builder`]'s connection return values on failure.
    /// * Forwards [`get_or_create_resource_group_id`]'s return values on failure.
    /// * Forwards [`Self::fetch_dispatched_pending_jobs`]'s return values on failure.
    /// * Forwards [`Self::fetch_submitted_running_jobs`]'s return values on failure.
    pub async fn new(
        coordinator_config: &CoordinatorConfig,
        spider_config: &SpiderConfig,
        db_pool: sqlx::MySqlPool,
        db_config: DatabaseConfig,
    ) -> Result<(Self, CancellationToken), Error> {
        let max_concurrent_tasks = coordinator_config.max_concurrent_tasks.get();
        if max_concurrent_tasks > Semaphore::MAX_PERMITS {
            return Err(Error::InvalidConfiguration(format!(
                "`max_concurrent_tasks` must not exceed {}, got {max_concurrent_tasks}",
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
            compression_task_max_retry: coordinator_config.compression_task_max_retry,
            commit_task_execution_policy: ExecutionPolicy {
                max_num_instances: 1,
                max_num_retry: coordinator_config.commit_task_max_retry,
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
            db_config,
            spider_option,
            job_polling_interval: Duration::from_millis(
                coordinator_config.job_polling_interval_millisecs.get(),
            ),
            cancellation_token: cancellation_token.clone(),
            job_handler_sem: Arc::new(Semaphore::new(max_concurrent_tasks)),
        };

        coordinator.recover_previous_jobs().await?;

        Ok((coordinator, cancellation_token))
    }

    /// Runs the coordinator's polling loop until cancelled.
    ///
    /// Each polling iteration consists of three phases:
    ///
    /// 1. Schedule pending compression jobs up to the available concurrency limit.
    /// 2. Wait until the next polling interval or until cancellation.
    /// 3. Mark the scheduled jobs as dispatched.
    ///
    /// `dispatch_time` marks jobs that have already been dispatched by the current coordinator,
    /// preventing them from being dispatched again before their handlers persist the `Running`
    /// state. These updates are batched and applied after the polling interval to reduce
    /// contention with the handlers' `Running` state updates.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::schedule_new_jobs`]'s return values on failure.
    /// * Forwards [`Self::mark_jobs_dispatched`]'s return values on failure.
    pub async fn run(self) -> Result<(), Error> {
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

    /// Recovers compression jobs left over from a previous coordinator instance.
    ///
    /// Picks up jobs in two states:
    ///
    /// * [`CompressionJobStatus::Running`] rows with a Spider job ID — the previous coordinator
    ///   submitted them to Spider and the handler is resumed via
    ///   [`S3CompressionJobHandle::recover`].
    /// * [`CompressionJobStatus::Pending`] rows whose `dispatch_time` is populated — the previous
    ///   coordinator claimed them but died before the handler's `Running` write landed, so they are
    ///   re-dispatched via [`S3CompressionJobHandle::run`].
    ///
    /// Each job is spawned as a detached handler. There is no concurrency limit for recovery, so
    /// the number of recovered jobs may temporarily exceed the configured limit if the coordinator
    /// is restarted with a lower limit.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::fetch_submitted_running_jobs`]'s return values on failure.
    /// * Forwards [`Self::fetch_dispatched_pending_jobs`]'s return values on failure.
    /// * Forwards [`Self::create_job_handle`]'s return values on failure.
    async fn recover_previous_jobs(&self) -> Result<(), Error> {
        let mut recovery_rows = self.fetch_submitted_running_jobs().await?;
        recovery_rows.extend(self.fetch_dispatched_pending_jobs().await?);

        for row in recovery_rows {
            let job_id = row.id;
            let spider_job_id = row.spider_job_id;
            let Some(clp_io_config) = self
                .try_deserialize_clp_io_config(job_id, &row.serialized_clp_io_config)
                .await
            else {
                continue;
            };

            tracing::info!(
                job_id = % job_id,
                spider_job_id = ? spider_job_id,
                "Recovering a previously submitted job."
            );
            let Ok(job_handle) = self.create_job_handle(job_id, clp_io_config).await else {
                continue;
            };

            let permit = self.job_handler_sem.clone().try_acquire_owned().ok();
            tokio::spawn(async move {
                let _permit = permit;
                let result = match spider_job_id {
                    Some(id) => job_handle.recover(id).await,
                    None => job_handle.run().await,
                };
                if let Err(e) = result {
                    tracing::error!(
                        error = % e,
                        job_id = % job_id,
                        "The recovered compression job failed."
                    );
                }
            });
        }

        Ok(())
    }

    /// Marks the compression job identified by `job_id` as [`CompressionJobStatus::Failed`].
    ///
    /// This is a best-effort update; if it fails, the error is logged and otherwise ignored.
    async fn mark_job_failed(&self, job_id: CompressionJobId, status_msg: &str) {
        const QUERY: &str = formatcp!(
            "UPDATE `{table}` SET `status` = ?, `status_msg` = ?, `update_time` = \
             CURRENT_TIMESTAMP() WHERE `id` = ?;",
            table = COMPRESSION_JOB_TABLE_NAME,
        );
        tracing::info!(job_id = % job_id, "Failing the compression job.");
        if let Err(e) = sqlx::query(QUERY)
            .bind(CompressionJobStatus::Failed)
            .bind(status_msg)
            .bind(job_id)
            .execute(&self.db_pool)
            .await
        {
            tracing::error!(
                error = % e,
                job_id = % job_id,
                "Failed to mark the compression job as failed."
            );
        }
    }

    /// Fetches up to the configured concurrency limit of pending jobs and spawns a detached
    /// handler for each.
    ///
    /// Jobs with invalid configurations or whose handles cannot be constructed are marked
    /// [`CompressionJobStatus::Failed`] and skipped. Jobs with unsupported input configurations are
    /// left pending for the legacy Celery-based compression scheduler.
    ///
    /// # Returns
    ///
    /// The IDs of the fetched jobs that were dispatched in this poll.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::fetch_new_job_rows`]'s return values on failure.
    async fn schedule_new_jobs(&self) -> Result<Vec<CompressionJobId>, Error> {
        let available_permits = self.job_handler_sem.available_permits();
        if available_permits == 0 {
            return Ok(Vec::new());
        }

        let new_job_rows = self
            .fetch_new_job_rows(available_permits)
            .await
            .inspect_err(|e| {
                tracing::error!(error = % e, "Failed to fetch new jobs from database.");
            })?;

        let mut dispatched_job_ids = Vec::with_capacity(new_job_rows.len());
        for job_row in new_job_rows {
            let Ok(permit) = self.job_handler_sem.clone().try_acquire_owned() else {
                break;
            };

            let job_id = job_row.id;
            dispatched_job_ids.push(job_id);

            let Some(clp_io_config) = self
                .try_deserialize_clp_io_config(job_id, &job_row.serialized_clp_io_config)
                .await
            else {
                continue;
            };

            tracing::info!(job_id = % job_id, "Scheduling new job.");
            let Ok(job_handle) = self.create_job_handle(job_id, clp_io_config).await else {
                continue;
            };

            tokio::spawn(async move {
                let _permit = permit;
                let _ = job_handle.run().await.inspect_err(|e| {
                    tracing::error!(
                        error = % e,
                        job_id = % job_id,
                        "Failed to schedule S3 compression job."
                    );
                });
            });
        }
        Ok(dispatched_job_ids)
    }

    /// Marks the compression jobs identified by `job_ids` with the current dispatch time.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    async fn mark_jobs_dispatched(&self, job_ids: &[CompressionJobId]) -> Result<(), Error> {
        if job_ids.is_empty() {
            return Ok(());
        }

        let mut tx = self.db_pool.begin().await?;
        for chunk in job_ids.chunks(1000) {
            let mut query_builder = sqlx::QueryBuilder::<sqlx::MySql>::new(formatcp!(
                "UPDATE `{table}` SET `dispatch_time` = CURRENT_TIMESTAMP() WHERE `id` IN (",
                table = COMPRESSION_JOB_TABLE_NAME,
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

    /// Constructs an [`S3CompressionJobHandle`] for the given job.
    ///
    /// A construction failure is logged, and the job is marked [`CompressionJobStatus::Failed`] for
    /// any failure other than an unsupported input config, which is only warned and left for
    /// another handler.
    ///
    /// # Returns
    ///
    /// The constructed [`S3CompressionJobHandle`] on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`S3CompressionJobHandle::new`]'s return values on failure.
    async fn create_job_handle(
        &self,
        job_id: CompressionJobId,
        clp_io_config: ClpIoConfig,
    ) -> Result<S3CompressionJobHandle<SpiderClient>, Error> {
        let result = S3CompressionJobHandle::new(
            self.db_pool.clone(),
            self.db_config.clone(),
            job_id,
            self.spider_client.clone(),
            self.resource_group_id,
            clp_io_config,
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
                    "Failed to create S3 job handle. Skipping."
                );
                self.mark_job_failed(
                    job_id,
                    &format!("Failed to create the compression job handle: {e}"),
                )
                .await;
            }
        }

        result
    }

    /// Fetches pending compression jobs that are ready to be dispatched.
    ///
    /// Returns up to `limit` [`CompressionJobStatus::Pending`] jobs that have not yet been
    /// dispatched, ordered by ascending job ID.
    ///
    /// # Returns
    ///
    /// A vector of rows projected from the compression job table on success, each row represents a
    /// pending compression job.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    async fn fetch_new_job_rows(&self, limit: usize) -> Result<Vec<JobRowProjection>, Error> {
        const QUERY: &str = formatcp!(
            "SELECT `id`, NULL AS `spider_id`, `clp_config` FROM `{table}` WHERE `status` = ? AND \
             `dispatch_time` IS NULL ORDER BY `id` ASC LIMIT ?;",
            table = COMPRESSION_JOB_TABLE_NAME,
        );

        let rows = sqlx::query_as::<_, JobRowProjection>(QUERY)
            .bind(CompressionJobStatus::Pending)
            .bind(i64::try_from(limit).map_err(|_| {
                Error::InvalidConfiguration(format!("`limit` must fit in i64, got {limit}"))
            })?)
            .fetch_all(&self.db_pool)
            .await?;

        Ok(rows)
    }

    /// Fetches jobs that are still in [`CompressionJobStatus::Running`] and were submitted by a
    /// previous compression coordinator instance.
    ///
    /// # Returns
    ///
    /// A vector of raw rows projected from the compression job table on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    async fn fetch_submitted_running_jobs(&self) -> Result<Vec<JobRowProjection>, Error> {
        const QUERY: &str = formatcp!(
            "SELECT `id`, `spider_id`, `clp_config` FROM `{table}` WHERE `status` = ? AND \
             `spider_id` IS NOT NULL;",
            table = COMPRESSION_JOB_TABLE_NAME,
        );

        let rows = sqlx::query_as::<_, JobRowProjection>(QUERY)
            .bind(CompressionJobStatus::Running)
            .fetch_all(&self.db_pool)
            .await?;

        Ok(rows)
    }

    /// Fetches pending jobs that were dispatched by a previous coordinator instance.
    ///
    /// These jobs have a `dispatch_time` but remain [`CompressionJobStatus::Pending`], indicating
    /// that their handlers did not successfully persist the `Running` state and therefore have
    /// not begun processing.
    ///
    /// # Returns
    ///
    /// A vector of raw rows projected from the compression job table on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    async fn fetch_dispatched_pending_jobs(&self) -> Result<Vec<JobRowProjection>, Error> {
        const QUERY: &str = formatcp!(
            "SELECT `id`, NULL AS `spider_id`, `clp_config` FROM `{table}` WHERE `status` = ? AND \
             `dispatch_time` IS NOT NULL;",
            table = COMPRESSION_JOB_TABLE_NAME,
        );

        let rows = sqlx::query_as::<_, JobRowProjection>(QUERY)
            .bind(CompressionJobStatus::Pending)
            .fetch_all(&self.db_pool)
            .await?;

        Ok(rows)
    }

    /// Deserializes `serialized_config` as a [`ClpIoConfig`].
    ///
    /// On failure, logs the error, marks the compression job as [`CompressionJobStatus::Failed`].
    ///
    /// # Returns
    ///
    /// The deserialized CLP io config.
    async fn try_deserialize_clp_io_config(
        &self,
        job_id: CompressionJobId,
        serialized_config: &[u8],
    ) -> Option<ClpIoConfig> {
        match BrotliMsgpack::deserialize(serialized_config) {
            Ok(clp_io_config) => Some(clp_io_config),
            Err(e) => {
                tracing::error!(
                    error = % e,
                    job_id = % job_id,
                    "Failed to deserialize CLP I/O config. Skipping."
                );
                self.mark_job_failed(
                    job_id,
                    &format!("Failed to deserialize CLP I/O config: {e}"),
                )
                .await;
                None
            }
        }
    }
}

const COMPRESSION_JOB_TABLE_NAME: &str = "compression_jobs";

/// A projection of the columns read from a compression job row.
#[derive(Debug, sqlx::FromRow)]
struct JobRowProjection {
    id: CompressionJobId,
    #[sqlx(rename = "spider_id")]
    spider_job_id: Option<SpiderJobId>,
    #[sqlx(rename = "clp_config")]
    serialized_clp_io_config: Vec<u8>,
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
