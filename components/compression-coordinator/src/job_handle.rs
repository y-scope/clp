//! Handle for driving a single compression job to completion.

use std::sync::Arc;
use std::time::Duration;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::COMPRESSION_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::ClpIoConfig;
use clp_rust_utils::job_config::CompressionJobId;
use clp_rust_utils::job_config::CompressionJobStatus;
use clp_rust_utils::job_config::FsInputConfig;
use clp_rust_utils::job_config::InputConfig;
use clp_rust_utils::job_config::S3ObjectMetadataInputConfig;
use clp_rust_utils::s3::ObjectMetadata;
use clp_rust_utils::s3::S3ObjectMetadataId;
use clp_rust_utils::task_io::compression::ClpSCompressionOption;
use clp_rust_utils::task_io::compression::FsInputSource;
use clp_rust_utils::task_io::compression::S3InputSource;
use const_format::formatcp;
use non_empty_string::NonEmptyString;
use spider_core::task::ExecutionPolicy;
use spider_core::task::TimeoutPolicy;
use spider_core::types::id::JobId as SpiderJobId;
use spider_core::types::id::ResourceGroupId;
use sqlx::MySqlPool;

use crate::Error;
use crate::compression_job_submitter::CompressionJobOutcome;
use crate::compression_job_submitter::CompressionJobSubmitter;
use crate::compression_job_submitter::CompressionManifest;
use crate::input::discover_fs_entries;
use crate::partition::CompressionInputBuilder;
use crate::partition::FsSource;

/// Options for a compression job running in Spider.
pub struct SpiderOption {
    pub compression_task_max_retry: u32,
    pub commit_task_execution_policy: ExecutionPolicy,
    pub initial_poll_backoff: Duration,
    pub max_poll_backoff: Duration,
}

/// Resources and execution options shared by compression job handles.
pub struct CompressionJobHandleContext {
    pub db_pool: MySqlPool,
    pub db_config: Database,

    /// The container's logs-input mount point, or `None` when filesystem input is unavailable.
    pub logs_input_mount_point: Option<NonEmptyString>,
    pub spider_option: SpiderOption,
}

/// Handles the asynchronous submission of a compression job and the retrieval of its result.
///
/// # Type Parameters
///
/// * `SubmitterType` - The type of the job submitter for Spider job submission.
pub struct CompressionJobHandle<SubmitterType: CompressionJobSubmitter> {
    context: Arc<CompressionJobHandleContext>,
    compression_job_id: CompressionJobId,
    job_submitter: SubmitterType,
    resource_group_id: ResourceGroupId,

    input_config: InputConfig,
    clp_s_compression_option: ClpSCompressionOption,
    dataset: DatasetName,
    target_archive_size: u64,
}

impl<SubmitterType: CompressionJobSubmitter> CompressionJobHandle<SubmitterType> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created [`CompressionJobHandle`] for the given compression job, with the `clp-s`
    /// compression options derived from `clp_io_config`'s output config.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::UnsupportedInputConfig`] if `clp_io_config` requests direct S3 discovery.
    pub fn new(
        context: Arc<CompressionJobHandleContext>,
        compression_job_id: CompressionJobId,
        job_submitter: SubmitterType,
        resource_group_id: ResourceGroupId,
        clp_io_config: ClpIoConfig,
    ) -> Result<Self, Error> {
        let input_config = clp_io_config.input;
        let (dataset, timestamp_key, unstructured) = match &input_config {
            InputConfig::S3ObjectMetadataInputConfig { config } => (
                config.dataset.clone(),
                config.timestamp_key.clone(),
                config.unstructured,
            ),
            InputConfig::FsInputConfig { config } => (
                config.dataset.clone(),
                config.timestamp_key.clone(),
                config.unstructured,
            ),
            InputConfig::S3InputConfig { .. } => return Err(Error::UnsupportedInputConfig),
        };

        let output_config = clp_io_config.output;
        let clp_s_compression_option = ClpSCompressionOption {
            target_encoded_size: output_config.target_segment_size
                + output_config.target_dictionaries_size,
            compression_level: output_config.compression_level,
            timestamp_key: timestamp_key.map(String::from),
            unstructured,
        };

        Ok(Self {
            context,
            compression_job_id,
            job_submitter,
            resource_group_id,
            input_config,
            clp_s_compression_option,
            dataset,
            target_archive_size: output_config.target_archive_size,
        })
    }

    /// Submits the compression job to Spider and drives it to completion.
    ///
    /// This method prepares the compression tasks' inputs, ensures the dataset's metadata tables
    /// exist, submits the job, persists the Spider job ID it was assigned, and then waits for the
    /// job to reach a terminal state. On any failure, the compression job is marked as
    /// [`CompressionJobStatus::Failed`] before the error is returned.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::submit_and_wait`]'s return values on failure.
    pub async fn run(self) -> Result<(), Error> {
        tracing::info!(compression_job_id = % self.compression_job_id, "Starting compression job.");

        let result = self.submit_and_wait().await;
        if let Err(err) = &result {
            self.report_failure(err).await;
        }

        result
    }

    /// Resumes a compression job that was already submitted to Spider.
    ///
    /// This method skips submission and waits for the Spider job identified by `spider_job_id` to
    /// reach a terminal state. On failure, the compression job is marked as
    /// [`CompressionJobStatus::Failed`] before the error is returned.
    ///
    /// NOTE: It's the caller's responsibility to ensure that the given Spider job ID is associated
    /// with the compression job.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::to_completion`]'s return values on failure.
    pub async fn recover(self, spider_job_id: SpiderJobId) -> Result<(), Error> {
        tracing::info!(
            compression_job_id = % self.compression_job_id,
            spider_job_id = % spider_job_id,
            "Recovering compression job.",
        );
        match self.to_completion(spider_job_id).await {
            Ok(()) => Ok(()),
            Err(err) => {
                self.report_failure(&err).await;
                Err(err)
            }
        }
    }

    /// Submits the compression job to Spider and waits for it to reach a terminal state.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::prepare_s3_task_inputs`]'s return values on failure.
    /// * Forwards [`Self::prepare_fs_task_inputs`]'s return values on failure.
    /// * Forwards [`Self::submit`]'s return values on failure.
    /// * Forwards [`Self::to_completion`]'s return values on failure.
    async fn submit_and_wait(&self) -> Result<(), Error> {
        let spider_job_id = match &self.input_config {
            InputConfig::S3ObjectMetadataInputConfig { config } => {
                self.submit(self.prepare_s3_task_inputs(config).await?)
                    .await?
            }
            InputConfig::FsInputConfig { config } => {
                self.submit(self.prepare_fs_task_inputs(config).await?)
                    .await?
            }
            InputConfig::S3InputConfig { .. } => {
                unreachable!("the input config is validated in the factory")
            }
        };
        self.to_completion(spider_job_id).await
    }

    /// Submits the compression job and persists its Spider association.
    ///
    /// # Type Parameters
    ///
    /// * `ManifestType` - The concrete input manifest consumed by each compression task.
    ///
    /// # Returns
    ///
    /// The submitted Spider job ID on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::upsert_metadata_tables`]'s return values on failure.
    /// * Forwards [`CompressionJobSubmitter::submit_compression_job`]'s return values on failure.
    /// * Forwards [`Self::persist_spider_job_id`]'s return values on failure.
    async fn submit<ManifestType: CompressionManifest>(
        &self,
        input_sources: Vec<(ManifestType, ExecutionPolicy)>,
    ) -> Result<SpiderJobId, Error> {
        let num_tasks = input_sources.len();

        self.upsert_metadata_tables().await?;

        let spider_job_id = self
            .job_submitter
            .submit_compression_job(
                self.compression_job_id,
                self.resource_group_id,
                self.clp_s_compression_option.clone(),
                self.dataset.clone(),
                input_sources,
                self.context
                    .spider_option
                    .commit_task_execution_policy
                    .clone(),
            )
            .await?;
        tracing::info!(
            compression_job_id = % self.compression_job_id,
            spider_job_id = % spider_job_id,
            "Compression job submitted.",
        );

        self.persist_spider_job_id(spider_job_id, num_tasks).await?;
        tracing::info!(
            compression_job_id = % self.compression_job_id,
            spider_job_id = % spider_job_id,
            "Compression job submission persisted.",
        );

        Ok(spider_job_id)
    }

    /// Reports a compression job failure.
    ///
    /// This method logs the original error and attempts to mark the compression job as
    /// [`CompressionJobStatus::Failed`] in the CLP database. The stored status message includes the
    /// original error message.
    ///
    /// If updating the job status fails, the status-update error is logged for observability and
    /// otherwise ignored.
    async fn report_failure(&self, err: &Error) {
        tracing::error!(
            compression_job_id = % self.compression_job_id,
            error = % err,
            "Compression job failed.",
        );
        let status_message = format!("Compression job failed: {err}");
        if let Err(e) = self
            .update_job_status(CompressionJobStatus::Failed, Some(status_message))
            .await
        {
            tracing::error!(
                compression_job_id = % self.compression_job_id,
                error = % e,
                "Failed to update job status on a job failure.",
            );
        }
    }

    /// Prepares the task inputs for the compression job.
    ///
    /// This method retrieves object metadata from the S3 object metadata table in the CLP database,
    /// partitions the objects into compression task inputs, and attaches the configured execution
    /// policy to each task.
    ///
    /// # Returns
    ///
    /// A vector of tuples on success, where each tuple contains:
    ///
    /// * An [`S3InputSource`] representing the input to a single compression task.
    /// * The [`ExecutionPolicy`] for that task.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::NoS3ObjectMetadata`] if the job doesn't request any object metadata IDs.
    /// * [`Error::DuplicateS3ObjectMetadata`] if the requested IDs contain a duplicate.
    /// * [`Error::MissingS3ObjectMetadata`] if a requested ID does not exist in the metadata table.
    /// * [`Error::EmptyS3ObjectMetadataField`] if a requested bucket or key is empty.
    /// * [`Error::NoTaskInputs`] if no compression task inputs are produced.
    /// * Forwards [`CompressionInputBuilder::add`]'s return values on failure.
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    async fn prepare_s3_task_inputs(
        &self,
        config: &S3ObjectMetadataInputConfig,
    ) -> Result<Vec<(S3InputSource, ExecutionPolicy)>, Error> {
        // NOTE: We batch the metadata fetch to avoid hitting the maximum placeholder limit of
        // MySQL.
        const FETCH_CHUNK_SIZE: usize = 1000;

        if config.s3_object_metadata_ids.is_empty() {
            return Err(Error::NoS3ObjectMetadata(config.ingestion_job_id));
        }

        let mut sorted_metadata_ids = config.s3_object_metadata_ids.clone();
        sorted_metadata_ids.sort_unstable();
        let mut duplicate_ids: Vec<S3ObjectMetadataId> = Vec::new();
        sorted_metadata_ids.dedup_by(|a, b| {
            if *a != *b {
                return false;
            }
            if duplicate_ids.last().copied() != Some(*b) {
                duplicate_ids.push(*b);
            }
            true
        });
        if !duplicate_ids.is_empty() {
            return Err(Error::DuplicateS3ObjectMetadata {
                ingestion_job_id: config.ingestion_job_id,
                ids: duplicate_ids,
            });
        }

        let mut input_builder =
            CompressionInputBuilder::new(config.s3_config.clone(), self.target_archive_size);
        for chunk in sorted_metadata_ids.chunks(FETCH_CHUNK_SIZE) {
            let mut query_builder = sqlx::QueryBuilder::<sqlx::MySql>::new(format!(
                "SELECT `id`, `bucket`, `key`, `size` FROM \
                 `{INGESTED_S3_OBJECT_METADATA_TABLE_NAME}` WHERE `id` IN ("
            ));
            let mut separated_ids = query_builder.separated(", ");
            for metadata_id in chunk {
                separated_ids.push_bind(metadata_id);
            }
            query_builder
                .push(") AND `ingestion_job_id` = ")
                .push_bind(config.ingestion_job_id);
            query_builder.push(" ORDER BY `id` ASC");

            let metadata_rows = query_builder
                .build_query_as::<S3ObjectMetadataRow>()
                .fetch_all(&self.context.db_pool)
                .await?;

            let mut row_iter = metadata_rows.into_iter().peekable();
            for &expected_id in chunk {
                if row_iter.peek().is_none_or(|row| row.id != expected_id) {
                    return Err(Error::MissingS3ObjectMetadata {
                        ingestion_job_id: config.ingestion_job_id,
                        id: expected_id,
                    });
                }
                let row = row_iter
                    .next()
                    .expect("row should have been validated by the previous peek");
                let bucket = NonEmptyString::new(row.bucket).map_err(|_| {
                    Error::EmptyS3ObjectMetadataField {
                        id: row.id,
                        field: "bucket",
                    }
                })?;
                let key = NonEmptyString::new(row.key).map_err(|_| {
                    Error::EmptyS3ObjectMetadataField {
                        id: row.id,
                        field: "key",
                    }
                })?;
                input_builder.add(ObjectMetadata {
                    bucket,
                    key,
                    size: row.size,
                })?;
            }
        }

        let partitions = input_builder.build();
        if partitions.is_empty() {
            return Err(Error::NoTaskInputs);
        }

        Ok(partitions
            .into_iter()
            .map(|partition| {
                let execution_policy = self.task_execution_policy(
                    partition.estimated_uncompressed_size,
                    partition.input.object_keys.len(),
                );
                (partition.input, execution_policy)
            })
            .collect())
    }

    /// Discovers and partitions filesystem inputs and attaches each task's execution policy.
    ///
    /// # Returns
    ///
    /// Regular-file manifests paired with their execution policies on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::NoFsInputPaths`] if no input paths are requested.
    /// * [`Error::InvalidConfiguration`] if no logs-input mount point is configured.
    /// * [`Error::NoTaskInputs`] if the selection contains no regular files.
    /// * Forwards [`discover_fs_entries`]'s return values on failure.
    /// * Forwards [`CompressionInputBuilder::add`]'s return values on failure.
    async fn prepare_fs_task_inputs(
        &self,
        config: &FsInputConfig,
    ) -> Result<Vec<(FsInputSource, ExecutionPolicy)>, Error> {
        if config.paths_to_compress.is_empty() {
            return Err(Error::NoFsInputPaths);
        }
        let mount_point = self.context.logs_input_mount_point.clone().ok_or_else(|| {
            Error::InvalidConfiguration(
                "filesystem compression requires a configured logs-input mount point".to_owned(),
            )
        })?;
        let entries = discover_fs_entries(mount_point, config.paths_to_compress.clone()).await?;
        let mut input_builder = CompressionInputBuilder::new(FsSource, self.target_archive_size);
        for entry in entries {
            input_builder.add(entry)?;
        }
        let partitions = input_builder.build();
        if partitions.is_empty() {
            return Err(Error::NoTaskInputs);
        }
        Ok(partitions
            .into_iter()
            .map(|partition| {
                let execution_policy = self.task_execution_policy(
                    partition.estimated_uncompressed_size,
                    partition.input.file_paths.len(),
                );
                (partition.input, execution_policy)
            })
            .collect())
    }

    /// Derives a compression task's retry and timeout policy from its input size and count.
    ///
    /// # Returns
    ///
    /// The configured retry policy with size-based timeouts and latency for at least one input.
    fn task_execution_policy(
        &self,
        estimated_uncompressed_size: u64,
        num_inputs: usize,
    ) -> ExecutionPolicy {
        ExecutionPolicy {
            max_num_retry: self.context.spider_option.compression_task_max_retry,
            timeout_policy: compression_timeout_policy(
                estimated_uncompressed_size,
                num_inputs as u64,
            ),
            ..ExecutionPolicy::default()
        }
    }

    /// Ensures that the required metadata tables exist for the configured dataset.
    ///
    /// This method creates the following tables in the CLP database if they do not already exist:
    ///
    /// * The archive metadata table.
    /// * The column metadata table.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::MetadataTableCreation`] if either table cannot be created.
    async fn upsert_metadata_tables(&self) -> Result<(), Error> {
        let archives_table = self.context.db_config.archives_table_name(&self.dataset);
        let column_metadata_table = self
            .context
            .db_config
            .column_metadata_table_name(&self.dataset);

        sqlx::query(&format!(
            "CREATE TABLE IF NOT EXISTS `{archives_table}` (
                `pagination_id` BIGINT unsigned NOT NULL AUTO_INCREMENT,
                `id` VARCHAR(64) NOT NULL,
                `begin_timestamp` BIGINT NOT NULL,
                `end_timestamp` BIGINT NOT NULL,
                `uncompressed_size` BIGINT NOT NULL,
                `size` BIGINT NOT NULL,
                `creator_id` VARCHAR(64) NOT NULL,
                `creation_ix` INT NOT NULL,
                KEY `archives_creation_order` (`creator_id`,`creation_ix`) USING BTREE,
                UNIQUE KEY `archive_id` (`id`) USING BTREE,
                PRIMARY KEY (`pagination_id`)
            )"
        ))
        .execute(&self.context.db_pool)
        .await
        .map_err(|source| Error::MetadataTableCreation {
            table: archives_table,
            source,
        })?;

        sqlx::query(&format!(
            "CREATE TABLE IF NOT EXISTS `{column_metadata_table}` (
                `name` VARCHAR(512) NOT NULL,
                `type` TINYINT NOT NULL,
                PRIMARY KEY (`name`, `type`)
            )"
        ))
        .execute(&self.context.db_pool)
        .await
        .map_err(|source| Error::MetadataTableCreation {
            table: column_metadata_table,
            source,
        })?;

        Ok(())
    }

    /// Persists the Spider job ID and marks the compression job as running.
    ///
    /// This method associates the given Spider job ID with the compression job in the CLP database
    /// and updates the compression job status to [`CompressionJobStatus::Running`].
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
    /// * [`Error::TooManyCompressionTasks`] if `num_tasks` exceeds `i32`'s range.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn persist_spider_job_id(
        &self,
        spider_job_id: SpiderJobId,
        num_tasks: usize,
    ) -> Result<(), Error> {
        let num_tasks =
            i32::try_from(num_tasks).map_err(|_| Error::TooManyCompressionTasks(num_tasks))?;
        sqlx::query(formatcp!(
            "UPDATE `{COMPRESSION_JOBS_TABLE_NAME}` SET `spider_id` = ?, `status` = ?, \
             `num_tasks` = ?, `start_time` = CURRENT_TIMESTAMP(3), `dispatch_time` = \
             COALESCE(`dispatch_time`, CURRENT_TIMESTAMP()) WHERE `id` = ?"
        ))
        .bind(spider_job_id.get())
        .bind(CompressionJobStatus::Running)
        .bind(num_tasks)
        .bind(self.compression_job_id)
        .execute(&self.context.db_pool)
        .await?;

        Ok(())
    }

    /// Waits for the associated Spider job to complete and finalizes the compression job.
    ///
    /// This method monitors the specified Spider job until it reaches a terminal state, then
    /// updates the compression job according to the Spider job's result.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`CompressionJobSubmitter::run_compression_job_to_completion`]'s return values on
    ///   failure.
    /// * Forwards [`Self::update_job_status`]'s return values on failure.
    /// * Forwards [`Self::get_job_status`]'s return values on failure.
    async fn to_completion(&self, spider_job_id: SpiderJobId) -> Result<(), Error> {
        let outcome = self
            .job_submitter
            .run_compression_job_to_completion(
                spider_job_id,
                self.context.spider_option.initial_poll_backoff,
                self.context.spider_option.max_poll_backoff,
            )
            .await?;
        tracing::info!(
            compression_job_id = % self.compression_job_id,
            spider_job_id = % spider_job_id,
            outcome = ? outcome,
            "Compression job reached a terminal state.",
        );

        match outcome {
            // The commit task records the successful CLP job status and publishes the archives in
            // the same transaction.
            CompressionJobOutcome::Succeeded => Ok(()),
            CompressionJobOutcome::Failed { error_message } => {
                if CompressionJobStatus::Succeeded == self.get_job_status().await? {
                    // NOTE: The commit task may successfully proceed but fail to report to Spider's
                    // control unit. In that case, the compression outcome has already committed to
                    // CLP DB, and thus the job should be considered `Succeeded`.
                    return Ok(());
                }
                self.update_job_status(
                    CompressionJobStatus::Failed,
                    Some(format!(
                        "The Spider compression job failed: {error_message}"
                    )),
                )
                .await
            }
            CompressionJobOutcome::Cancelled => {
                if CompressionJobStatus::Succeeded == self.get_job_status().await? {
                    // NOTE: The commit task may successfully proceed but fail to report to Spider's
                    // control unit. In that case, the compression outcome has already committed to
                    // CLP DB, and thus the job should be considered `Succeeded`.
                    return Ok(());
                }
                self.update_job_status(
                    CompressionJobStatus::Killed,
                    Some("The Spider compression job was cancelled.".to_owned()),
                )
                .await
            }
        }
    }

    /// Reads the current status of the compression job from the CLP database.
    ///
    /// # Returns
    ///
    /// The current [`CompressionJobStatus`] of the compression job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryScalar::fetch_one`]'s return values on failure.
    async fn get_job_status(&self) -> Result<CompressionJobStatus, Error> {
        let job_status: CompressionJobStatus = sqlx::query_scalar(formatcp!(
            "SELECT `status` FROM `{COMPRESSION_JOBS_TABLE_NAME}` WHERE `id` = ?"
        ))
        .bind(self.compression_job_id)
        .fetch_one(&self.context.db_pool)
        .await?;

        Ok(job_status)
    }

    /// Updates the compression job status in the CLP database.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn update_job_status(
        &self,
        job_status: CompressionJobStatus,
        status_message: Option<String>,
    ) -> Result<(), Error> {
        let status_message = status_message.as_ref().map_or("", String::as_str);
        sqlx::query(formatcp!(
            "UPDATE `{COMPRESSION_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = ? WHERE `id` \
             = ?"
        ))
        .bind(job_status)
        .bind(status_message)
        .bind(self.compression_job_id)
        .execute(&self.context.db_pool)
        .await?;

        Ok(())
    }
}

const INGESTED_S3_OBJECT_METADATA_TABLE_NAME: &str = "ingested_s3_object_metadata";

/// A projection of the columns read from an ingested S3 object metadata row.
#[derive(Debug, sqlx::FromRow)]
struct S3ObjectMetadataRow {
    id: S3ObjectMetadataId,
    bucket: String,
    key: String,
    size: u64,
}

fn compression_timeout_policy(estimated_uncompressed_size: u64, num_files: u64) -> TimeoutPolicy {
    const SOFT_THROUGHPUT_BYTES_PER_SEC: u64 = 40 * 1024 * 1024;
    const HARD_THROUGHPUT_BYTES_PER_SEC: u64 = 8 * 1024 * 1024;
    const FILE_LATENCY_MS: u64 = 1000;
    const MIN_TIMEOUT_MS: u64 = 100;
    const MAX_TIMEOUT_MS: u64 = 24 * 60 * 60 * 1000;

    let latency_ms = num_files.max(1).saturating_mul(FILE_LATENCY_MS);
    let timeout_ms = |throughput| {
        estimated_uncompressed_size
            .saturating_mul(1000)
            .div_ceil(throughput)
            .saturating_add(latency_ms)
    };
    let soft_timeout_ms =
        timeout_ms(SOFT_THROUGHPUT_BYTES_PER_SEC).clamp(MIN_TIMEOUT_MS, MAX_TIMEOUT_MS - 1);
    let hard_timeout_ms =
        timeout_ms(HARD_THROUGHPUT_BYTES_PER_SEC).clamp(soft_timeout_ms + 1, MAX_TIMEOUT_MS);
    TimeoutPolicy {
        soft_timeout_ms,
        hard_timeout_ms,
    }
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::logs_input::LogsInputPath;
    use spider_core::task::TaskGraph;
    use spider_core::task::TdlContext;
    use spider_core::task::TerminationTaskDescriptor;

    use super::*;
    use crate::partition::FsEntry;
    use crate::partition::FsSource;

    #[test]
    fn compression_timeouts_scale_with_size_and_file_latency() -> anyhow::Result<()> {
        for (size, num_files, soft_timeout_ms, hard_timeout_ms) in [
            (0, 0, 1000, 1001),
            (0, 1, 1000, 1001),
            (1, 1, 1001, 1002),
            (40 * 1024 * 1024, 2, 3000, 7000),
            (1024 * 1024 * 1024 * 1024, 1, 26_215_400, 86_400_000),
            (u64::MAX, 1, 86_399_999, 86_400_000),
        ] {
            let policy = compression_timeout_policy(size, num_files);
            assert_eq!(
                policy,
                TimeoutPolicy {
                    soft_timeout_ms,
                    hard_timeout_ms
                }
            );
            TaskGraph::new(
                Some(TerminationTaskDescriptor {
                    tdl_context: TdlContext {
                        package: "clp".to_owned(),
                        task_func: "compression::commit".to_owned(),
                    },
                    execution_policy: Some(ExecutionPolicy {
                        timeout_policy: policy,
                        ..ExecutionPolicy::default()
                    }),
                }),
                None,
            )?;
        }
        Ok(())
    }

    #[test]
    fn full_filesystem_partition_has_valid_size_based_timeouts() -> anyhow::Result<()> {
        const TARGET_ARCHIVE_SIZE: u64 = 256 * 1024 * 1024;
        const NUM_FILES: u64 = 4096;

        let mut builder = CompressionInputBuilder::new(FsSource, TARGET_ARCHIVE_SIZE);
        for index in 0..NUM_FILES {
            builder.add(FsEntry {
                path: LogsInputPath::try_from(format!("/logs/{index}"))?,
                size: TARGET_ARCHIVE_SIZE / NUM_FILES,
            })?;
        }
        let partitions = builder.build();
        assert_eq!(partitions.len(), 1);
        let partition = &partitions[0];
        assert_eq!(partition.input.file_paths.len() as u64, NUM_FILES);
        let policy = compression_timeout_policy(
            partition.estimated_uncompressed_size,
            partition.input.file_paths.len() as u64,
        );
        assert_eq!(
            policy,
            TimeoutPolicy {
                soft_timeout_ms: 4_102_400,
                hard_timeout_ms: 4_128_000,
            }
        );
        TaskGraph::new(
            Some(TerminationTaskDescriptor {
                tdl_context: TdlContext {
                    package: "clp".to_owned(),
                    task_func: "compression::commit".to_owned(),
                },
                execution_policy: Some(ExecutionPolicy {
                    timeout_policy: policy,
                    ..ExecutionPolicy::default()
                }),
            }),
            None,
        )?;
        Ok(())
    }
}
