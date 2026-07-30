//! Handle for driving a single S3 compression job to completion.

use std::sync::Arc;
use std::time::Duration;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::dataset::VALID_DATASET_NAME_REGEX;
use clp_rust_utils::job_config::ClpIoConfig;
use clp_rust_utils::job_config::CompressionJobId;
use clp_rust_utils::job_config::CompressionJobStatus;
use clp_rust_utils::job_config::InputConfig;
use clp_rust_utils::s3::ObjectMetadata;
use clp_rust_utils::s3::S3ObjectMetadataId;
use clp_rust_utils::task_io::compression::ClpSCompressionOption;
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
use crate::compression_job_submitter::S3CompressionJobSubmitter;
use crate::partition::CompressionInputBuilder;

/// Options for a compression job running in Spider.
pub struct SpiderOption {
    pub compression_task_max_retry: u32,
    pub commit_task_execution_policy: ExecutionPolicy,
    pub initial_poll_backoff: Duration,
    pub max_poll_backoff: Duration,
}

/// Handles the asynchronous submission of an S3 compression job and the retrieval of its result.
///
/// # Type Parameters
///
/// * `SubmitterType` - The type of the job submitter for Spider job submission.
pub struct S3CompressionJobHandle<SubmitterType: S3CompressionJobSubmitter> {
    db_pool: MySqlPool,
    db_config: Database,
    compression_job_id: CompressionJobId,
    job_submitter: SubmitterType,
    resource_group_id: ResourceGroupId,

    input_config: InputConfig,
    clp_s_compression_option: ClpSCompressionOption,
    dataset: Option<String>,
    target_archive_size: u64,

    spider_option: Arc<SpiderOption>,
}

impl<SubmitterType: S3CompressionJobSubmitter> S3CompressionJobHandle<SubmitterType> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created [`S3CompressionJobHandle`] for the given compression job, with the `clp-s`
    /// compression options derived from `clp_io_config`'s output config.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::UnsupportedInputConfig`] if `clp_io_config`'s input config is not an
    ///   [`InputConfig::S3ObjectMetadataInputConfig`].
    /// * [`Error::InvalidDataset`] if the configured dataset name is not a valid dataset name.
    pub fn new(
        db_pool: MySqlPool,
        db_config: Database,
        compression_job_id: CompressionJobId,
        job_submitter: SubmitterType,
        resource_group_id: ResourceGroupId,
        clp_io_config: ClpIoConfig,
        spider_option: Arc<SpiderOption>,
    ) -> Result<Self, Error> {
        let input_config = clp_io_config.input;
        let InputConfig::S3ObjectMetadataInputConfig {
            config: s3_object_metadata_config,
        } = &input_config
        else {
            return Err(Error::UnsupportedInputConfig);
        };
        let dataset: Option<String> = s3_object_metadata_config.dataset.clone().map(String::from);
        if let Some(dataset_name) = dataset.as_ref()
            && !VALID_DATASET_NAME_REGEX.is_match(dataset_name)
        {
            return Err(Error::InvalidDataset(dataset_name.clone()));
        }

        let output_config = clp_io_config.output;
        let clp_s_compression_option = ClpSCompressionOption {
            target_encoded_size: output_config.target_segment_size
                + output_config.target_dictionaries_size,
            compression_level: output_config.compression_level,
            timestamp_key: s3_object_metadata_config
                .timestamp_key
                .clone()
                .map(String::from),
            unstructured: s3_object_metadata_config.unstructured,
        };

        Ok(Self {
            db_pool,
            db_config,
            compression_job_id,
            job_submitter,
            resource_group_id,
            input_config,
            clp_s_compression_option,
            dataset,
            target_archive_size: output_config.target_archive_size,
            spider_option,
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
    /// * Forwards [`Self::prepare_task_inputs`]'s return values on failure.
    /// * Forwards [`Self::upsert_metadata_tables`]'s return values on failure.
    /// * Forwards [`S3CompressionJobSubmitter::submit_s3_compression_job`]'s return values on
    ///   failure.
    /// * Forwards [`Self::persist_spider_job_id`]'s return values on failure.
    /// * Forwards [`Self::to_completion`]'s return values on failure.
    async fn submit_and_wait(&self) -> Result<(), Error> {
        let input_sources = self.prepare_task_inputs().await?;
        let num_tasks = input_sources.len();

        self.upsert_metadata_tables().await?;

        let spider_job_id = self
            .job_submitter
            .submit_s3_compression_job(
                self.compression_job_id,
                self.resource_group_id,
                self.clp_s_compression_option.clone(),
                self.dataset.clone(),
                input_sources,
                self.spider_option.commit_task_execution_policy.clone(),
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

        self.to_completion(spider_job_id).await
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
    async fn prepare_task_inputs(&self) -> Result<Vec<(S3InputSource, ExecutionPolicy)>, Error> {
        // NOTE: We batch the metadata fetch to avoid hitting the maximum placeholder limit of
        // MySQL.
        const FETCH_CHUNK_SIZE: usize = 1000;

        let InputConfig::S3ObjectMetadataInputConfig { config } = &self.input_config else {
            unreachable!("the input config is validated in the factory")
        };
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

        let mut input_builder = CompressionInputBuilder::from_s3_config(
            config.s3_config.clone(),
            self.target_archive_size,
        );
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
                .fetch_all(&self.db_pool)
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

        let input_sources = input_builder.into_task_input_sources();
        if input_sources.is_empty() {
            return Err(Error::NoTaskInputs);
        }

        Ok(input_sources
            .into_iter()
            .map(|input_source| {
                // NOTE: The timeout policy scales with the number of objects assigned to the task.
                // Each object contributes three minutes to the soft timeout and five minutes to
                // the hard timeout. This heuristic does not account for object size and can be
                // refined in the future.
                let num_objects = input_source.object_keys.len() as u64;
                let timeout_policy = TimeoutPolicy {
                    soft_timeout_ms: num_objects * 3 * 60 * 1000,
                    hard_timeout_ms: num_objects * 5 * 60 * 1000,
                };
                let execution_policy = ExecutionPolicy {
                    max_num_retry: self.spider_option.compression_task_max_retry,
                    timeout_policy,
                    ..ExecutionPolicy::default()
                };
                (input_source, execution_policy)
            })
            .collect())
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
        let archives_table = self.db_config.archives_table_name(self.dataset.as_deref());
        let column_metadata_table = self
            .db_config
            .column_metadata_table_name(self.dataset.as_deref());

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
        .execute(&self.db_pool)
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
        .execute(&self.db_pool)
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
            "UPDATE `{COMPRESSION_JOB_TABLE_NAME}` SET `spider_id` = ?, `status` = ?, `num_tasks` \
             = ?, `start_time` = CURRENT_TIMESTAMP(3) WHERE `id` = ?"
        ))
        .bind(spider_job_id.get())
        .bind(CompressionJobStatus::Running)
        .bind(num_tasks)
        .bind(self.compression_job_id)
        .execute(&self.db_pool)
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
    /// * Forwards [`S3CompressionJobSubmitter::run_s3_compression_job_to_completion`]'s return
    ///   values on failure.
    /// * Forwards [`Self::update_job_status`]'s return values on failure.
    /// * Forwards [`Self::get_job_status`]'s return values on failure.
    async fn to_completion(&self, spider_job_id: SpiderJobId) -> Result<(), Error> {
        let outcome = self
            .job_submitter
            .run_s3_compression_job_to_completion(
                spider_job_id,
                self.spider_option.initial_poll_backoff,
                self.spider_option.max_poll_backoff,
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
            "SELECT `status` FROM `{COMPRESSION_JOB_TABLE_NAME}` WHERE `id` = ?"
        ))
        .bind(self.compression_job_id)
        .fetch_one(&self.db_pool)
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
            "UPDATE `{COMPRESSION_JOB_TABLE_NAME}` SET `status` = ?, `status_msg` = ? WHERE `id` \
             = ?"
        ))
        .bind(job_status)
        .bind(status_message)
        .bind(self.compression_job_id)
        .execute(&self.db_pool)
        .await?;

        Ok(())
    }
}

const COMPRESSION_JOB_TABLE_NAME: &str = "compression_jobs";
const INGESTED_S3_OBJECT_METADATA_TABLE_NAME: &str = "ingested_s3_object_metadata";

/// A projection of the columns read from an ingested S3 object metadata row.
#[derive(Debug, sqlx::FromRow)]
struct S3ObjectMetadataRow {
    id: S3ObjectMetadataId,
    bucket: String,
    key: String,
    size: u64,
}
