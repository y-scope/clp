//! The coordinator poll loop that discovers pending CLP compression jobs and dispatches them to
//! Spider.

use std::{sync::Arc, time::Duration};

use clp_rust_utils::{
    clp_config::package::config::{
        CompressionCoordinator as CoordinatorConfig,
        Database as DatabaseConfig,
        Spider as SpiderConfig,
        SpiderResourceGroup,
    },
    job_config::{ClpIoConfig, CompressionJobId, CompressionJobStatus},
    serde::BrotliMsgpack,
};
use const_format::formatcp;
use spider_client::SpiderClient;
use spider_core::{
    task::{ExecutionPolicy, TimeoutPolicy},
    types::id::{JobId as SpiderJobId, ResourceGroupId},
};
use tokio::{select, time::Instant};
use tokio_util::sync::CancellationToken;
use tonic::transport::Endpoint;

use crate::{
    Error,
    job_handle::{S3CompressionJobHandle, SpiderOption},
};

/// Coordinator for fetching new compression jobs and submitting them to Spider.
pub struct Coordinator {
    resource_group_id: ResourceGroupId,
    spider_client: SpiderClient,
    db_pool: sqlx::MySqlPool,
    db_config: DatabaseConfig,
    spider_option: Arc<SpiderOption>,
    last_polled_job_id: Option<CompressionJobId>,
    job_polling_interval: Duration,
    cancellation_token: CancellationToken,
}

impl Coordinator {
    /// Factory function.
    ///
    /// On construction, this recovers compression jobs that a previous coordinator instance had
    /// already submitted to Spider (those still [`CompressionJobStatus::Running`] with a Spider job
    /// ID) by spawning a detached handle to drive each one to completion.
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
    /// * [`Error::InvalidEndpoint`] if the Spider host and port do not form a valid endpoint.
    /// * Forwards [`SpiderClient::builder`]'s connection return values on failure.
    /// * Forwards [`get_or_create_resource_group_id`]'s return values on failure.
    /// * Forwards [`Self::fetch_submitted_running_jobs`]'s return values on failure.
    pub async fn new(
        coordinator_config: &CoordinatorConfig,
        spider_config: &SpiderConfig,
        db_pool: sqlx::MySqlPool,
        db_config: DatabaseConfig,
    ) -> Result<(Self, CancellationToken), Error> {
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
            last_polled_job_id: None,
            job_polling_interval: Duration::from_millis(
                coordinator_config.job_polling_interval_millisecs.get(),
            ),
            cancellation_token: cancellation_token.clone(),
        };

        for (job_id, spider_job_id, clp_io_config) in
            coordinator.fetch_submitted_running_jobs().await?
        {
            tracing::info!(
                job_id = % job_id,
                spider_job_id = % spider_job_id,
                "Recovering a previously submitted job."
            );
            let Ok(job_handle) = coordinator.create_job_handle(job_id, clp_io_config).await else {
                continue;
            };
            tokio::spawn(async move {
                let _ = job_handle.recover(spider_job_id).await.inspect_err(|e| {
                    tracing::error!(
                        error = % e,
                        job_id = % job_id,
                        spider_job_id = % spider_job_id,
                        "The recovered compression job failed."
                    );
                });
            });
        }

        Ok((coordinator, cancellation_token))
    }

    /// Runs the coordinator's poll loop until cancelled.
    ///
    /// On each iteration, this method fetches the pending compression jobs, spawns a detached
    /// handle to drive each one, and then sleeps until the next poll or until the cancellation
    /// token is triggered.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::schedule_new_jobs`]'s return values on failure.
    pub async fn run(mut self) -> Result<(), Error> {
        let cancellation_token = self.cancellation_token.clone();
        loop {
            let now = Instant::now();

            select! {
                () = cancellation_token.cancelled() => {
                    break;
                }
                result = self.schedule_new_jobs() => {
                    result.inspect_err(|e| {
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
        }

        tracing::info!("Coordinator shutting down.");
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

    /// Fetches the pending compression jobs and spawns a detached handle to drive each one.
    ///
    ///
    /// A job whose config cannot be deserialized is marked [`CompressionJobStatus::Failed`] and
    /// skipped; a job whose handle cannot be constructed is skipped as well (and marked
    /// [`CompressionJobStatus::Failed`] unless its input config is unsupported, in which case it is
    /// left for the legacy Celery-based compression scheduler).
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::fetch_new_job_rows`]'s return values on failure.
    async fn schedule_new_jobs(&mut self) -> Result<(), Error> {
        let new_job_rows = self.fetch_new_job_rows().await.inspect_err(|e| {
            tracing::error!(error = % e, "Failed to fetch new jobs from database.");
        })?;
        for job_row in new_job_rows {
            let job_id = job_row.id;
            let clp_io_config: ClpIoConfig =
                match BrotliMsgpack::deserialize(&job_row.serialized_clp_io_config) {
                    Ok(clp_io_config) => clp_io_config,
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
                        continue;
                    }
                };
            tracing::info!(job_id = % job_id, "Scheduling new job.");
            let Ok(job_handle) = self.create_job_handle(job_id, clp_io_config).await else {
                continue;
            };
            tokio::spawn(async move {
                let _ = job_handle.run().await.inspect_err(|e| {
                    tracing::error!(
                        error = % e,
                        job_id = % job_id,
                        "Failed to schedule S3 compression job."
                    );
                });
            });
        }
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

    /// Fetches the pending compression jobs newer than the last polled job and advances the poll
    /// cursor.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    async fn fetch_new_job_rows(&mut self) -> Result<Vec<PendingJobRowProjection>, Error> {
        let query = self.last_polled_job_id.map_or_else(
            || {
                sqlx::query_as::<_, PendingJobRowProjection>(formatcp!(
                    "SELECT `id`, `clp_config` FROM `{table}` WHERE `status` = ? ORDER BY `id` \
                     ASC;",
                    table = COMPRESSION_JOB_TABLE_NAME,
                ))
                .bind(CompressionJobStatus::Pending)
            },
            |last_polled_job_id| {
                sqlx::query_as::<_, PendingJobRowProjection>(formatcp!(
                    "SELECT `id`, `clp_config` FROM `{table}` WHERE `status` = ? AND `id` > ? \
                     ORDER BY `id` ASC;",
                    table = COMPRESSION_JOB_TABLE_NAME,
                ))
                .bind(CompressionJobStatus::Pending)
                .bind(last_polled_job_id)
            },
        );

        let rows = query.fetch_all(&self.db_pool).await?;
        if let Some(last_row) = rows.last() {
            self.last_polled_job_id = Some(last_row.id);
        }

        Ok(rows)
    }

    /// Fetches jobs that are still in [`CompressionJobStatus::Running`] and were previously
    /// submitted by the compression coordinator.
    ///
    /// A running job whose config cannot be deserialized is marked [`CompressionJobStatus::Failed`]
    /// and skipped.
    ///
    /// # Returns
    ///
    /// A vector of tuples on success, each tuple containing:
    ///
    /// * The compression job ID.
    /// * The Spider job ID.
    /// * The IO config of the compression job.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    async fn fetch_submitted_running_jobs(
        &self,
    ) -> Result<Vec<(CompressionJobId, SpiderJobId, ClpIoConfig)>, Error> {
        const QUERY: &str = formatcp!(
            "SELECT `id`, `spider_id`, `clp_config` FROM `{table}` WHERE `status` = ? AND \
             `spider_id` IS NOT NULL;",
            table = COMPRESSION_JOB_TABLE_NAME,
        );

        let mut recovery_context = Vec::new();
        for row in sqlx::query_as::<_, RunningJobRowProjection>(QUERY)
            .bind(CompressionJobStatus::Running)
            .fetch_all(&self.db_pool)
            .await?
        {
            let clp_io_config: ClpIoConfig =
                match BrotliMsgpack::deserialize(&row.serialized_clp_io_config) {
                    Ok(clp_io_config) => clp_io_config,
                    Err(e) => {
                        tracing::error!(
                            error = % e,
                            job_id = % row.id,
                            "Failed to deserialize CLP I/O config of a running job. The database \
                             might be corrupted. Skipping."
                        );
                        self.mark_job_failed(
                            row.id,
                            &format!("Failed to deserialize CLP I/O config: {e}"),
                        )
                        .await;
                        continue;
                    }
                };
            recovery_context.push((row.id, row.spider_job_id, clp_io_config));
        }

        Ok(recovery_context)
    }
}

const COMPRESSION_JOB_TABLE_NAME: &str = "compression_jobs";

/// A projection of the columns read from a [`CompressionJobStatus::Pending`] compression job row.
#[derive(Debug, sqlx::FromRow)]
struct PendingJobRowProjection {
    id: CompressionJobId,
    #[sqlx(rename = "clp_config")]
    serialized_clp_io_config: Vec<u8>,
}

/// A projection of the columns read from a [`CompressionJobStatus::Running`] compression job row.
#[derive(Debug, sqlx::FromRow)]
struct RunningJobRowProjection {
    id: CompressionJobId,
    #[sqlx(rename = "spider_id")]
    spider_job_id: SpiderJobId,
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
