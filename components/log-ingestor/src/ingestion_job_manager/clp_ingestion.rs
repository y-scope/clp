use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::{
    clp_config::{
        AwsAuthentication,
        AwsCredentials,
        package::{
            config::{ArchiveOutput, Config as ClpConfig, LogsInput},
            credentials::Credentials as ClpCredentials,
        },
    },
    database::mysql::MySqlEnumFormat,
    impl_sqlx_type,
    job_config::{
        ClpIoConfig,
        CompressionJobId,
        CompressionJobStatus,
        InputConfig,
        ingestion::s3::{S3IngestionJobConfig, S3ScannerConfig},
    },
    s3::{ObjectMetadata, S3ObjectMetadataId},
};
use const_format::formatcp;
use non_empty_string::NonEmptyString;
use sqlx::MySqlPool;
use strum_macros::{AsRefStr, Display, EnumIter, EnumString};
use tokio::sync::mpsc;

use crate::{
    compression::{
        Buffer,
        CLP_COMPRESSION_JOB_TABLE_NAME,
        CompressionJobSubmitter,
        Listener,
        wait_for_compression_job_completion_and_update_metadata,
    },
    ingestion_job::{IngestionJobState, S3ScannerState, SqsListenerState},
    ingestion_job_manager::{IngestionJobId, TerminalStatus},
};

/// A bundle of objects for log-ingestor to recovery from a restart.
pub struct LogIngestorRecoveryContext {
    /// A vector of contexts for all unfinished compression jobs previously submitted to CLP DB.
    pub unfinished_compression_jobs: Vec<ClpCompressionJobContext>,

    /// A vector of contexts for all recoverable ingestion jobs in CLP DB. These ingestion jobs are
    /// in either [`ClpIngestionJobStatus::Requested`] or [`ClpIngestionJobStatus::Running`]
    /// status, and can be resumed by creating an ingestion job instance with the context.
    pub recoverable_ingestion_jobs: Vec<ClpIngestionJobContext>,

    /// A vector of contexts for all inactive ingestion jobs in CLP DB. These ingestion jobs don't
    /// have active ingestion job instances, but for any already-ingested but still buffered object
    /// metadata, there should be a compression listener to re-submit them for compression.
    pub inactive_ingestion_jobs: Vec<ClpIngestionJobContext>,
}

/// A bundle of objects to manage an already-submitted CLP compression job.
pub struct ClpCompressionJobContext {
    compression_state: ClpCompressionState,
    compression_job_id: CompressionJobId,
    num_object_metadata_submitted: usize,
}

impl ClpCompressionJobContext {
    #[must_use]
    pub const fn get_compression_job_id(&self) -> CompressionJobId {
        self.compression_job_id
    }

    /// Creates a detached coroutine to wait for the compression job to complete, and updates the
    /// status of all ingested
    pub fn detach_and_wait_for_completion_and_update_metadata(self) {
        tokio::spawn(async move {
            wait_for_compression_job_completion_and_update_metadata(
                self.compression_state,
                self.compression_job_id,
                self.num_object_metadata_submitted,
            )
            .await;
        });
    }
}

/// A bundle of objects to manage CLP ingestion jobs.
pub struct ClpIngestionJobContext {
    config: S3IngestionJobConfig,
    ingestion_state: ClpIngestionState,
    compression_state: ClpCompressionState,
    listener: Listener,
}

impl ClpIngestionJobContext {
    #[must_use]
    pub const fn get_job_id(&self) -> IngestionJobId {
        self.ingestion_state.get_job_id()
    }

    /// # Returns
    ///
    /// A clone of the underlying ingestion state.
    #[must_use]
    pub fn get_ingestion_state(&self) -> ClpIngestionState {
        self.ingestion_state.clone()
    }

    /// # Returns
    ///
    /// A clone of the underlying compression state.
    #[must_use]
    pub fn get_compression_state(&self) -> ClpCompressionState {
        self.compression_state.clone()
    }

    /// # Returns
    ///
    /// A new sender for buffer ingestion.
    #[must_use]
    pub fn get_ingestion_buffer_sender(&self) -> mpsc::Sender<Vec<ObjectMetadata>> {
        self.listener.get_new_sender()
    }

    #[must_use]
    pub const fn get_ingestion_job_config(&self) -> &S3IngestionJobConfig {
        &self.config
    }

    /// Shuts down the CLP ingestion.
    ///
    /// # Returns
    ///
    /// The terminal status of the job indicating whether it has stopped with an error.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`ClpIngestionState::get_job_status`]'s return values on failure.
    /// * Forwards [`ClpIngestionState::end`]'s return values on failure.
    pub async fn shutdown(self) -> anyhow::Result<TerminalStatus> {
        self.listener.shutdown_and_join().await;
        if ClpIngestionJobStatus::Failed == self.ingestion_state.get_job_status().await? {
            return Ok(TerminalStatus::Failed);
        }
        self.ingestion_state.end().await.inspect_err(|e| {
            tracing::error!(
                job_id = ? self.ingestion_state.job_id,
                error = ? e,
                "Failed to end ingestion job while the job instance has been removed."
            );
        })?;
        Ok(TerminalStatus::Finished)
    }
}

/// Connector for managing ingestion jobs in CLP DB.
pub struct ClpDbIngestionConnector {
    db_pool: MySqlPool,
    channel_capacity: usize,
    aws_credentials: AwsCredentials,
    archive_output_config: ArchiveOutput,
    buffer_flush_timeout: Duration,
    buffer_flush_threshold: u64,
}

impl ClpDbIngestionConnector {
    /// Creates a connection to CLP DB for ingestion.
    ///
    /// # Returns
    ///
    /// A tuple on success, containing:
    ///
    /// * A newly created instance of [`ClpDbIngestionConnector`] connected to CLP DB with the given
    ///   configuration and credentials.
    /// * A context for log-ingestor to recover from a restart. For details, check the documentation
    ///   of [`LogIngestorRecoveryContext`].
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`clp_rust_utils::database::mysql::create_clp_db_mysql_pool`]'s return values on
    ///   failure.
    /// * Forwards [`Self::create_tables`]'s return values on failure.
    /// * Forwards [`Self::get_unfinished_compression_jobs`]'s return values on failure.
    /// * Forwards [`Self::load_ingestion_jobs`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if the logs input type is unsupported ([`LogsInput::Fs`]).
    pub async fn connect(
        clp_config: ClpConfig,
        clp_credentials: ClpCredentials,
    ) -> anyhow::Result<(Self, LogIngestorRecoveryContext)> {
        let log_ingestor_config = clp_config
            .log_ingestor
            .as_ref()
            .expect("log_ingestor configuration is missing");

        let aws_credentials = match clp_config.logs_input {
            LogsInput::S3 { config } => match config.aws_authentication {
                AwsAuthentication::Credentials { credentials } => credentials,
            },
            LogsInput::Fs { .. } => {
                panic!(
                    "Invalid CLP config: Unsupported logs input type. The current implementation \
                     only supports S3 input."
                );
            }
        };

        let mysql_pool = clp_rust_utils::database::mysql::create_clp_db_mysql_pool(
            &clp_config.database,
            &clp_credentials.database,
            100,
        )
        .await?;

        Self::create_tables(&mysql_pool).await?;

        let connector = Self {
            db_pool: mysql_pool,
            channel_capacity: log_ingestor_config.channel_capacity,
            aws_credentials,
            archive_output_config: clp_config.archive_output.clone(),
            buffer_flush_timeout: Duration::from_secs(log_ingestor_config.buffer_flush_timeout_sec),
            buffer_flush_threshold: log_ingestor_config.buffer_flush_threshold,
        };

        let unfinished_compression_jobs = connector.get_unfinished_compression_jobs().await?;
        let (recoverable_ingestion_jobs, inactive_ingestion_jobs) =
            connector.load_ingestion_jobs().await?;
        let recovery_context = LogIngestorRecoveryContext {
            unfinished_compression_jobs,
            recoverable_ingestion_jobs,
            inactive_ingestion_jobs,
        };
        Ok((connector, recovery_context))
    }

    /// Creates a new ingestion job in the CLP database with the given configuration.
    ///
    /// # Returns
    ///
    /// A [`ClpIngestionJobContext`] instance representing the newly created ingestion job on
    /// success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    /// * Forwards [`serde_json::to_string`]'s return values on failure.
    pub async fn create_ingestion_job(
        &self,
        config: S3IngestionJobConfig,
    ) -> anyhow::Result<ClpIngestionJobContext> {
        const QUERY: &str = formatcp!(
            r"INSERT INTO `{table}` (`config`) VALUES (?);",
            table = INGESTION_JOB_TABLE_NAME,
        );

        let mut tx = self.db_pool.begin().await?;
        let job_id = sqlx::query(QUERY)
            .bind(serde_json::to_string(&config)?)
            .execute(&mut *tx)
            .await?
            .last_insert_id();

        if let S3IngestionJobConfig::S3Scanner(_) = &config {
            const S3_SCANNER_STATE_INSERT_QUERY: &str = formatcp!(
                r"INSERT INTO `{table}` (`id`) VALUES (?);",
                table = INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME,
            );
            sqlx::query(S3_SCANNER_STATE_INSERT_QUERY)
                .bind(job_id)
                .execute(&mut *tx)
                .await?;
        }

        tx.commit().await?;

        Ok(self.create_clp_ingestion_context(job_id, config))
    }

    /// Gets the status of an ingestion job with the given ID from CLP DB.
    ///
    /// # Returns
    ///
    /// The status of the ingestion job with the given ID if it exists in CLP DB, `None` otherwise.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::fetch_optional`]'s return values on failure.
    pub async fn get_job_status(
        &self,
        job_id: IngestionJobId,
    ) -> anyhow::Result<Option<ClpIngestionJobStatus>> {
        let status: Option<ClpIngestionJobStatus> = sqlx::query_scalar(formatcp!(
            r"SELECT `status` FROM `{table}` WHERE `id` = ?;",
            table = INGESTION_JOB_TABLE_NAME,
        ))
        .bind(job_id)
        .fetch_optional(&self.db_pool)
        .await?;
        Ok(status)
    }

    /// Attempts to fail an ingestion job with the given ID.
    ///
    /// # NOTE
    ///
    /// This method doesn't perform any validation on the job ID before attempting to fail it, which
    /// means the job ID may not exist in the CLP DB.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`update_job_status`]'s return values on failure.
    pub async fn try_fail(&self, job_id: IngestionJobId, error_msg: String) -> anyhow::Result<()> {
        update_job_status(
            self.db_pool.clone(),
            job_id,
            ClpIngestionJobStatus::Failed,
            Some(&error_msg),
        )
        .await
    }

    /// Retrieves all unfinished compression jobs that are previously submitted.
    ///
    /// # Returns
    ///
    /// A vector of tuples on success, where each tuple contains:
    ///
    /// * The compression state associated with the compression job.
    /// * The ID of an unfinished compression job.
    /// * The number of metadata submitted for the compression job.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure when failing to
    ///   fetch already-submitted compression jobs.
    async fn get_unfinished_compression_jobs(
        &self,
    ) -> anyhow::Result<Vec<ClpCompressionJobContext>> {
        const QUERY: &str = formatcp!(
            "SELECT `ingestion_job_id`, `compression_job_id`, COUNT(*) as `num_submitted` FROM \
             `{table}` WHERE `compression_job_id` IS NOT NULL AND `status` = ? GROUP BY \
             `ingestion_job_id`, `compression_job_id` ORDER BY `compression_job_id` ASC;",
            table = INGESTED_S3_OBJECT_METADATA_TABLE_NAME,
        );
        let all_rows: Vec<(IngestionJobId, CompressionJobId, i64)> = sqlx::query_as(QUERY)
            .bind(IngestedS3ObjectMetadataStatus::Submitted)
            .fetch_all(&self.db_pool)
            .await?;
        let all = all_rows
            .into_iter()
            .map(
                |(ingestion_job_id, compression_job_id, num_submitted)| ClpCompressionJobContext {
                    compression_state: ClpCompressionState {
                        ingestion_job_id,
                        db_pool: self.db_pool.clone(),
                    },
                    compression_job_id,
                    num_object_metadata_submitted: usize::try_from(num_submitted)
                        .expect("Number of files submitted is not `usize` compatible"),
                },
            )
            .collect();
        Ok(all)
    }

    /// Creates all required DB tables for CLP ingestion.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure if:
    ///   * [`INGESTION_JOB_TABLE_NAME`] table creation query execution fails.
    ///   * [`INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME`] table creation query execution fails.
    ///   * [`INGESTED_S3_OBJECT_METADATA_TABLE_NAME`] table creation query execution fails.
    async fn create_tables(db_pool: &MySqlPool) -> anyhow::Result<()> {
        sqlx::query(ingestion_job_table_creation_query().as_str())
            .execute(db_pool)
            .await?;

        sqlx::query(ingestion_job_s3_scanner_state_table_creation_query().as_str())
            .execute(db_pool)
            .await?;

        sqlx::query(ingested_s3_object_metadata_table_creation_query().as_str())
            .execute(db_pool)
            .await?;

        Ok(())
    }

    /// Creates S3 ingestion context for the given ingestion job ID.
    ///
    /// # Returns
    ///
    /// A [`ClpIngestionJobContext`] instance, with a newly created listener for receiving ingested
    /// object metadata for compression job submission.
    fn create_clp_ingestion_context(
        &self,
        job_id: IngestionJobId,
        config: S3IngestionJobConfig,
    ) -> ClpIngestionJobContext {
        let compression_state = ClpCompressionState {
            ingestion_job_id: job_id,
            db_pool: self.db_pool.clone(),
        };

        let submitter = CompressionJobSubmitter::new(
            compression_state.clone(),
            self.aws_credentials.clone(),
            &self.archive_output_config,
            config.as_base_config(),
        );

        let listener = Listener::spawn(
            Buffer::new(submitter, self.buffer_flush_threshold),
            self.buffer_flush_timeout,
            self.channel_capacity,
        );
        let ingestion_state = ClpIngestionState {
            job_id,
            db_pool: self.db_pool.clone(),
            sender: listener.get_new_sender(),
        };

        ClpIngestionJobContext {
            config,
            ingestion_state,
            compression_state,
            listener,
        }
    }

    /// Loads ingestion contexts for all ingestion jobs from CLP DB.
    ///
    /// # NOTE
    ///
    /// This method loads ingestion jobs in a best-effort manner. It there is an error when loading
    /// the job config, that error will be logged and the job will be skipped.
    ///
    /// # Returns
    ///
    /// A tuple on success, containing:
    ///
    /// * A vector of [`ClpIngestionJobContext`] instances representing all recoverable ingestion
    ///   jobs
    /// * A vector of [`ClpIngestionJobContext`] instances representing all inactive ingestion jobs.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure when fetching job
    ///   metadata.
    async fn load_ingestion_jobs(
        &self,
    ) -> anyhow::Result<(Vec<ClpIngestionJobContext>, Vec<ClpIngestionJobContext>)> {
        // Load recoverable ingestion jobs.
        let mut recoverable_ingestion_jobs: Vec<ClpIngestionJobContext> = Vec::new();
        let requested_jod_id_and_config: Vec<(IngestionJobId, String)> = sqlx::query_as(formatcp!(
            "SELECT `id`, `config` FROM `{table}` WHERE `status` = ?;",
            table = INGESTION_JOB_TABLE_NAME,
        ))
        .bind(ClpIngestionJobStatus::Requested)
        .fetch_all(&self.db_pool)
        .await
        .inspect_err(|e| {
            tracing::error!(
                error = ? e,
                "Failed to fetch requested ingestion jobs from CLP DB during service start."
            );
        })?;
        for (job_id, config_str) in requested_jod_id_and_config {
            let Some(config) = self.load_config(job_id, config_str).await else {
                tracing::error!(
                    job_id = ? job_id,
                    "Failed to load config for a requested ingestion job at service start. The job \
                        will be skipped."
                );
                continue;
            };
            recoverable_ingestion_jobs.push(self.create_clp_ingestion_context(job_id, config));
        }

        let running_jod_id_and_config: Vec<(IngestionJobId, String)> = sqlx::query_as(formatcp!(
            "SELECT `id`, `config` FROM `{table}` WHERE `status` = ?;",
            table = INGESTION_JOB_TABLE_NAME,
        ))
        .bind(ClpIngestionJobStatus::Running)
        .fetch_all(&self.db_pool)
        .await
        .inspect_err(|e| {
            tracing::error!(
                error = ? e,
                "Failed to fetch running ingestion jobs from CLP DB during service start."
            );
        })?;

        for (job_id, config_str) in running_jod_id_and_config {
            let Some(config) = self.load_config(job_id, config_str).await else {
                continue;
            };
            let config = match config {
                S3IngestionJobConfig::S3Scanner(config) => {
                    let updated_config = match self.update_start_after(job_id, config).await {
                        Ok(updated_config) => updated_config,
                        Err(e) => {
                            if let Err(e) = self.try_fail(job_id, e.to_string()).await {
                                tracing::error!(
                                    error = ? e,
                                    job_id = ? job_id,
                                    "Failed to update ingestion job status to `failed` for a S3 \
                                        scanner ingestion job with invalid last ingested key \
                                        during service start."
                                );
                            }
                            continue;
                        }
                    };
                    S3IngestionJobConfig::S3Scanner(updated_config)
                }
                other @ S3IngestionJobConfig::SqsListener(_) => other,
            };
            recoverable_ingestion_jobs.push(self.create_clp_ingestion_context(job_id, config));
        }

        // Load inactive ingestion jobs.
        let inactive_jod_id_and_config: Vec<(IngestionJobId, String)> = sqlx::query_as(formatcp!(
            "SELECT `id`, `config` FROM `{table}` WHERE `status` NOT in (?, ?);",
            table = INGESTION_JOB_TABLE_NAME,
        ))
        .bind(ClpIngestionJobStatus::Requested)
        .bind(ClpIngestionJobStatus::Running)
        .fetch_all(&self.db_pool)
        .await
        .inspect_err(|e| {
            tracing::error!(
            error = ? e,
            "Failed to fetch inactive ingestion jobs from CLP DB during service start."
            );
        })?;
        let mut inactive_ingestion_jobs: Vec<ClpIngestionJobContext> =
            Vec::with_capacity(inactive_jod_id_and_config.len());
        for (job_id, config_str) in inactive_jod_id_and_config {
            let Some(config) = self.load_config(job_id, config_str).await else {
                tracing::error!(
                    job_id = ? job_id,
                    "Failed to load config for an inactive ingestion job at service start. The job \
                        will be skipped."
                );
                continue;
            };
            inactive_ingestion_jobs.push(self.create_clp_ingestion_context(job_id, config));
        }

        Ok((recoverable_ingestion_jobs, inactive_ingestion_jobs))
    }

    /// Loads the configuration of an ingestion job with the given ID from CLP DB.
    ///
    /// # Returns
    ///
    /// The ingestion job configuration on success, or `None` if the config cannot be deserialized.
    /// On failure, the errors will be logged and the ingestion job will be marked as failed in CLP
    /// DB.
    async fn load_config(
        &self,
        job_id: IngestionJobId,
        config_str: String,
    ) -> Option<S3IngestionJobConfig> {
        match serde_json::from_str(&config_str) {
            Ok(config) => Some(config),
            Err(e) => {
                tracing::error!(
                    error = ? e,
                    job_id = ? job_id,
                    "Failed to parse ingestion job config from CLP DB during service start. \
                        The job will be skipped."
                );
                if let Err(e) = self
                    .try_fail(job_id, format!("Failed to parse ingestion job config: {e}"))
                    .await
                {
                    tracing::error!(
                        error = ? e,
                        job_id = ? job_id,
                        "Failed to update ingestion job status to `failed` for a job with invalid \
                            config during service start."
                    );
                }
                None
            }
        }
    }

    /// Given a job ID of a S3 scanner job and its config, updates `start_after` by reading the last
    /// ingested key from CLP DB (if any).
    ///
    /// # Returns
    ///
    /// The up-to-date S3 scanner config on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if the query fails to fetch the last ingested key for the given S3
    ///   scanner job.
    /// * [`anyhow::Error`] if the last ingested key fetched from CLP DB is an empty string.
    async fn update_start_after(
        &self,
        job_id: IngestionJobId,
        mut config: S3ScannerConfig,
    ) -> anyhow::Result<S3ScannerConfig> {
        let last_ingested_key = sqlx::query_scalar::<_, Option<String>>(formatcp!(
            r"SELECT `last_ingested_key` FROM `{table}` WHERE `id` = ?;",
            table = INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME,
        ))
        .bind(job_id)
        .fetch_one(&self.db_pool)
        .await
        .map_err(|e| {
            const ERROR_MSG: &str = "Failed to fetch last ingested key for a running S3 scanner \
                                     ingestion job from CLP DB.";
            tracing::error!(
                job_id = ? job_id,
                error = ? e,
                "Failed to fetch last ingested key for a running S3 scanner ingestion job \
                    from CLP DB."
            );
            anyhow::anyhow!(ERROR_MSG)
        })?;
        if let Some(last_ingested_key) = last_ingested_key {
            config.start_after = Some(NonEmptyString::new(last_ingested_key).map_err(|_| {
                anyhow::anyhow!("Invalid last ingested key stored in CLP DB: empty string")
            })?);
        }
        Ok(config)
    }
}

/// A CLP-DB-backed implementation of ingestion job state. This state ingests object metadata into
/// the CLP system and persists all ingestion progress and state transitions in the CLP database.
#[derive(Clone)]
pub struct ClpIngestionState {
    job_id: IngestionJobId,
    db_pool: MySqlPool,
    sender: mpsc::Sender<Vec<ObjectMetadata>>,
}

impl ClpIngestionState {
    #[must_use]
    pub const fn get_job_id(&self) -> IngestionJobId {
        self.job_id
    }

    /// Retrieves the current status of the underlying ingestion job from the CLP database.
    ///
    /// # Returns
    ///
    /// The current status of the underlying ingestion job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
    pub async fn get_job_status(&self) -> anyhow::Result<ClpIngestionJobStatus> {
        const QUERY: &str = formatcp!(
            r"SELECT `status` FROM `{table}` WHERE `id` = ?;",
            table = INGESTION_JOB_TABLE_NAME,
        );

        let status: ClpIngestionJobStatus = sqlx::query_scalar(QUERY)
            .bind(self.job_id)
            .fetch_one(&self.db_pool)
            .await?;

        Ok(status)
    }

    /// Gets all buffered S3 object metadata ingested for the underlying ingestion job from CLP DB.
    ///
    /// # Returns
    ///
    /// A vector of [`ObjectMetadata`] representing all ingested S3 object metadata in
    /// [`IngestedS3ObjectMetadataStatus::Buffered`] for the underlying ingestion job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if any fetched bucket name or key is an empty string.
    /// * Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure.
    pub async fn get_buffered_object_metadata(&self) -> anyhow::Result<Vec<ObjectMetadata>> {
        const QUERY: &str = formatcp!(
            "SELECT `id`, `bucket`, `key`, `size` FROM `{table}` WHERE `ingestion_job_id` = ? AND \
             `status` = ?;",
            table = INGESTED_S3_OBJECT_METADATA_TABLE_NAME,
        );

        let metadata_records =
            sqlx::query_as::<_, (S3ObjectMetadataId, String, String, u64)>(QUERY)
                .bind(self.job_id)
                .bind(IngestedS3ObjectMetadataStatus::Buffered)
                .fetch_all(&self.db_pool)
                .await?;

        let mut object_metadata_vec = Vec::with_capacity(metadata_records.len());
        for (id, bucket, key, size) in metadata_records {
            let bucket = NonEmptyString::new(bucket).map_err(|_| {
                anyhow::anyhow!("Invalid bucket name stored in CLP DB: empty string")
            })?;
            let key = NonEmptyString::new(key).map_err(|_| {
                anyhow::anyhow!("Invalid object key stored in CLP DB: empty string")
            })?;
            object_metadata_vec.push(ObjectMetadata {
                id: Some(id),
                bucket,
                key,
                size,
            });
        }

        Ok(object_metadata_vec)
    }

    /// Ingests the given S3 object metadata into CLP DB.
    ///
    /// # Returns
    ///
    /// A tuple on success, containing:
    ///
    /// * The chunk size used for batched metadata insertion.
    /// * A vector of IDs for the newly ingested S3 object metadata. Each ID denotes the first
    ///   record inserted in its respective batch. IDs within a single batch are guaranteed to form
    ///   a consecutive sequence.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// This method panics if:
    ///
    /// * `objects` is empty, as it cannot build a valid ingestion query in that case.
    async fn ingest_s3_object_metadata(
        &self,
        tx: &mut sqlx::Transaction<'_, sqlx::MySql>,
        objects: &[ObjectMetadata],
    ) -> anyhow::Result<(usize, Vec<S3ObjectMetadataId>)> {
        const BASE_INGESTION_QUERY: &str = formatcp!(
            r"INSERT INTO `{table}` (`bucket`, `key`, `size`, `ingestion_job_id`) VALUES ",
            table = INGESTED_S3_OBJECT_METADATA_TABLE_NAME,
        );

        const CHUNK_SIZE: usize = 10000;

        assert!(
            !objects.is_empty(),
            "Cannot build S3 object metadata ingestion query with empty objects"
        );

        let mut last_inserted_ids: Vec<S3ObjectMetadataId> = Vec::new();

        // Ingest object metadata
        // NOTE: MySQL has a maximum placeholder limit of 65535. We need to batch the ingestion to
        // avoid hitting this limit. If the number of placeholders per insert changes, we may need
        // to adjust the chunk size accordingly.
        for chunk in objects.chunks(CHUNK_SIZE) {
            let query_string = format!(
                "{}{}",
                BASE_INGESTION_QUERY,
                std::iter::repeat_n("(?, ?, ?, ?)", chunk.len())
                    .collect::<Vec<_>>()
                    .join(", ")
            );

            let mut query = sqlx::query(&query_string);
            for object in chunk {
                query = query
                    .bind(object.bucket.as_str())
                    .bind(object.key.as_str())
                    .bind(object.size)
                    .bind(self.job_id);
            }

            last_inserted_ids.push(query.execute(&mut **tx).await?.last_insert_id());
        }

        Ok((CHUNK_SIZE, last_inserted_ids))
    }

    /// Commits the transaction if the underlying job is valid and in
    /// [`ClpIngestionJobStatus::Running`] state.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success with the transaction committed.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if the job is not in the running state.
    /// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    async fn check_job_status_and_commit(
        &self,
        mut tx: sqlx::Transaction<'_, sqlx::MySql>,
    ) -> anyhow::Result<()> {
        let curr_status: ClpIngestionJobStatus = sqlx::query_scalar(formatcp!(
            r"SELECT `status` FROM `{table}` WHERE `id` = ? LOCK IN SHARE MODE;",
            table = INGESTION_JOB_TABLE_NAME,
        ))
        .bind(self.job_id)
        .fetch_one(&mut *tx)
        .await?;
        if curr_status != ClpIngestionJobStatus::Running {
            return Err(anyhow::anyhow!(
                "Job status update failed. The job may not exist or is not in the running state."
            ));
        }

        tx.commit().await?;
        Ok(())
    }

    /// Ingests the provided S3 object metadata and forwards it to the underlying object metadata
    /// channel.
    ///
    /// # NOTE
    ///
    /// All database operations are executed within the given transaction and are committed
    /// before the metadata is sent to the channel.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::ingest_s3_object_metadata`]'s return values on failure.
    /// * Forwards [`Self::check_job_status_and_commit`]'s return values on failure.
    /// * Forwards [`mpsc::Sender::send`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if the chunk IDs returned by [`Self::ingest_s3_object_metadata`] cannot be mapped to
    /// the corresponding entries in the `objects` vector.
    ///
    /// This condition indicates a violation of internal invariants and should never occur under
    /// normal execution unless the code has been corrupted or contains a logic error.
    async fn ingest_and_send(
        &self,
        mut tx: sqlx::Transaction<'_, sqlx::MySql>,
        mut objects: Vec<ObjectMetadata>,
    ) -> anyhow::Result<()> {
        let (chunk_size, last_inserted_ids) = self
            .ingest_s3_object_metadata(&mut tx, &objects)
            .await
            .inspect_err(|err| {
                tracing::error!(
                    error = ? err,
                    job_id = ? self.job_id,
                    "Failed to ingest S3 object metadata."
                );
            })?;

        self.check_job_status_and_commit(tx)
            .await
            .inspect_err(|err| {
                tracing::error!(
                    error = ? err,
                    job_id = ? self.job_id,
                    "Failed to commit ingestion."
                );
            })?;

        for (chunk_id, chunk) in objects.chunks_mut(chunk_size).enumerate() {
            for (next_metadata_id, object) in
                (*last_inserted_ids.get(chunk_id).expect("invalid chunk ID")..)
                    .zip(chunk.iter_mut())
            {
                object.id = Some(next_metadata_id);
            }
        }
        self.sender.send(objects).await?;
        Ok(())
    }
}

#[async_trait]
impl IngestionJobState for ClpIngestionState {
    /// # NOTE
    ///
    /// This method is idempotent.
    ///
    /// # Errors
    ///
    /// * Forwards [`update_job_status`]'s return values on failure.
    async fn start(&self) -> anyhow::Result<()> {
        update_job_status(
            self.db_pool.clone(),
            self.job_id,
            ClpIngestionJobStatus::Running,
            None,
        )
        .await
    }

    /// # NOTE
    ///
    /// This method is idempotent. It returns success if the underlying job is already in the
    /// terminated state.
    ///
    /// # Errors
    ///
    /// * Forwards [`update_job_status`]'s return values on failure.
    async fn end(&self) -> anyhow::Result<()> {
        update_job_status(
            self.db_pool.clone(),
            self.job_id,
            ClpIngestionJobStatus::Finished,
            None,
        )
        .await
    }

    /// # NOTE
    ///
    /// This method is idempotent. It returns success if the underlying job is already in the failed
    /// state. The previous error message will not be overwritten.
    ///
    /// # Errors
    ///
    /// * Forwards [`update_job_status`]'s return values on failure.
    async fn fail(&self, msg: String) {
        match update_job_status(
            self.db_pool.clone(),
            self.job_id,
            ClpIngestionJobStatus::Failed,
            Some(&msg),
        )
        .await
        {
            Ok(()) => {}
            Err(err) => {
                tracing::error!(
                    error = ? err,
                    status_msg = ? msg,
                    job_id = ? self.job_id,
                    "Failed to update job status to `failed`."
                );
            }
        }
    }
}

#[async_trait]
impl S3ScannerState for ClpIngestionState {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure when updating the last
    ///   ingested key for the S3 scanner state.
    /// * Forwards [`Self::ingest_and_send`]'s return values on failure.
    async fn ingest(
        &self,
        objects: Vec<ObjectMetadata>,
        last_ingested_key: &str,
    ) -> anyhow::Result<()> {
        const UPDATE_S3_SCANNER_STATE_QUERY: &str = formatcp!(
            r"UPDATE `{table}` SET `last_ingested_key` = ? WHERE `id` = ?;",
            table = INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME,
        );

        let mut tx = self.db_pool.begin().await?;

        if sqlx::query(UPDATE_S3_SCANNER_STATE_QUERY)
            .bind(last_ingested_key)
            .bind(self.job_id)
            .execute(&mut *tx)
            .await?
            .rows_affected()
            == 0
        {
            const ERROR_MSG: &str = "Failed to update last ingested key for S3 scanner state.";
            tracing::error!(
                job_id = ? self.job_id,
                last_ingested_key = ? last_ingested_key,
                ERROR_MSG
            );
            return Err(anyhow::anyhow!(ERROR_MSG));
        }

        self.ingest_and_send(tx, objects).await
    }
}

#[async_trait]
impl SqsListenerState for ClpIngestionState {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`Self::ingest_and_send`]'s return values on failure.
    async fn ingest(&self, objects: Vec<ObjectMetadata>) -> anyhow::Result<()> {
        self.ingest_and_send(self.db_pool.begin().await?, objects)
            .await
    }
}

/// A CLP-DB-backed implementation of compression job state.
///
/// This state manages the submission of compression jobs to CLP and the corresponding updates to
/// the ingested object metadata in the CLP database, on behalf of an ingestion job indicated by the
/// underlying ingestion job ID.
#[derive(Clone)]
pub struct ClpCompressionState {
    ingestion_job_id: IngestionJobId,
    db_pool: MySqlPool,
}

impl ClpCompressionState {
    /// # Returns
    ///
    /// The ID of the ingestion job that submits compression jobs using this state.
    #[must_use]
    pub const fn get_ingestion_job_id(&self) -> IngestionJobId {
        self.ingestion_job_id
    }

    /// Submits a compression job by populating the provided compression job configuration with the
    /// given CLP IO template and keys (where each key has an ingestion ID associated with it).
    ///
    /// # Returns
    ///
    /// The ID of the submitted compression job.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if the submitted compression job ID overflows.
    /// * [`anyhow::Error`] if one or more object metadata rows fail to be updated in the DB.
    /// * Forwards [`clp_rust_utils::serde::BrotliMsgpack::serialize`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if no object metadata is provided for compression.
    pub async fn submit_for_compression(
        &self,
        io_config_template: ClpIoConfig,
        id_and_key_pairs: Vec<(S3ObjectMetadataId, NonEmptyString)>,
    ) -> anyhow::Result<CompressionJobId> {
        // TODO: As tracked in #2018, once we support submitting compression jobs using IDs, there
        // is no need for passing keys into this method.
        const COMPRESSION_JOB_SUBMISSION_QUERY: &str = formatcp!(
            r"INSERT INTO {table} (`clp_config`) VALUES (?)",
            table = CLP_COMPRESSION_JOB_TABLE_NAME
        );

        if id_and_key_pairs.is_empty() {
            const ERROR_MSG: &str = "No objects to compress.";
            tracing::error!(job_id = ? self.ingestion_job_id, ERROR_MSG);
            panic!("{}", ERROR_MSG);
        }

        let mut io_config = io_config_template;
        let s3_input_config = match &mut io_config.input {
            InputConfig::S3InputConfig { config } => config,
        };
        s3_input_config.keys = Some(
            id_and_key_pairs
                .iter()
                .map(|(_, key)| key.clone())
                .collect(),
        );

        let mut tx = self.db_pool.begin().await?;

        // Submit compression job
        let result = sqlx::query(COMPRESSION_JOB_SUBMISSION_QUERY)
            .bind(clp_rust_utils::serde::BrotliMsgpack::serialize(&io_config)?)
            .execute(&mut *tx)
            .await?;
        let compression_job_id =
            CompressionJobId::try_from(result.last_insert_id()).map_err(|_| {
                anyhow::anyhow!("The retrieved ID overflows: {}", result.last_insert_id())
            })?;

        // Update compression job ID for ingested objects.
        // NOTE: We batch the update to avoid hitting the maximum placeholder limit of MySQL. The
        // batch size is chosen to be 10000, which is conservative enough to avoid hitting the limit
        // while also minimizing the number of batches for typical use cases. If the number of
        // placeholders per update changes, we may need to adjust the batch size accordingly.
        for chunk in id_and_key_pairs.chunks(10000) {
            let mut query_builder = sqlx::QueryBuilder::<sqlx::MySql>::new(formatcp!(
                r"UPDATE `{table}` ",
                table = INGESTED_S3_OBJECT_METADATA_TABLE_NAME,
            ));
            query_builder
                .push("SET `compression_job_id` = ")
                .push_bind(compression_job_id);
            query_builder
                .push(", `status` = ")
                .push_bind(IngestedS3ObjectMetadataStatus::Submitted);
            query_builder.push(" WHERE `id` IN (");
            let mut separated_ids = query_builder.separated(", ");
            for (id, _) in chunk {
                separated_ids.push_bind(id);
            }
            query_builder.push(")");
            query_builder
                .push(" AND `status` = ")
                .push_bind(IngestedS3ObjectMetadataStatus::Buffered);

            let result = query_builder.build().execute(&mut *tx).await?;
            if result.rows_affected()
                != u64::try_from(chunk.len()).expect("size conversion should always succeed")
            {
                return Err(anyhow::anyhow!(
                    "Failed to update compression job ID for some objects."
                ));
            }
        }

        tx.commit().await?;
        Ok(compression_job_id)
    }

    /// Waits for the compression job to finish and updates the status of submitted object metadata.
    ///
    /// # Returns
    ///
    /// A tuple on success, containing:
    ///
    /// * The status of the finished compression job.
    /// * The status message of the finished compression job, if any.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if one or more object metadata rows fail to be updated in the DB.
    /// * Forwards [`Self::wait_for_compression_result`]'s return values.
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if `num_metadata_submitted` overflows `u64`, which is unlikely in practice.
    pub async fn wait_for_compression_and_update_submitted_metadata(
        &self,
        id: CompressionJobId,
        num_metadata_submitted: usize,
    ) -> anyhow::Result<(CompressionJobStatus, Option<String>)> {
        const UPDATE_COMPRESSION_RESULT_QUERY: &str = formatcp!(
            r"UPDATE `{table}` SET `status` = ? WHERE `compression_job_id` = ?",
            table = INGESTED_S3_OBJECT_METADATA_TABLE_NAME
        );

        let (status, status_msg) = self.wait_for_compression_result(id).await?;

        let ingested_s3_object_metadata_status = match status {
            CompressionJobStatus::Succeeded => IngestedS3ObjectMetadataStatus::Compressed,
            CompressionJobStatus::Failed | CompressionJobStatus::Killed => {
                IngestedS3ObjectMetadataStatus::Failed
            }
            _ => {
                unreachable!(
                    "Compression job status returned from `Self::wait_for_compression_result` \
                     should be one of the above."
                )
            }
        };

        let mut tx = self.db_pool.begin().await?;
        if sqlx::query(UPDATE_COMPRESSION_RESULT_QUERY)
            .bind(ingested_s3_object_metadata_status)
            .bind(id)
            .execute(&mut *tx)
            .await?
            .rows_affected()
            != u64::try_from(num_metadata_submitted).expect("size conversion should always succeed")
        {
            return Err(anyhow::anyhow!(
                "Failed to update compression result for some objects."
            ));
        }

        if status == CompressionJobStatus::Succeeded {
            const INGESTION_JOB_UPDATE_QUERY: &str = formatcp!(
                r"UPDATE `{table}` SET `{field}` = `{field}` + ? WHERE `id` = ?",
                table = INGESTION_JOB_TABLE_NAME,
                field = "num_files_compressed"
            );
            // NOTE: We don't need to check the number of rows affected here because we don't need
            // to make any assumptions about the ingestion job status.
            sqlx::query(INGESTION_JOB_UPDATE_QUERY)
                .bind(
                    u64::try_from(num_metadata_submitted)
                        .expect("size conversion should always succeed"),
                )
                .bind(self.ingestion_job_id)
                .execute(&mut *tx)
                .await?;
        }

        tx.commit().await?;
        Ok((status, status_msg))
    }

    /// Waits for the compression job to finish. A compression job is considered finished when it's
    /// in one of the following states:
    ///
    /// * [`CompressionJobStatus::Succeeded`]
    /// * [`CompressionJobStatus::Failed`]
    /// * [`CompressionJobStatus::Killed`]
    ///
    /// # Returns
    ///
    /// A tuple on success, containing:
    ///
    /// * The status of the finished compression job.
    /// * The status message of the finished compression job, if any.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if the fetched status is unknown.
    /// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
    async fn wait_for_compression_result(
        &self,
        id: CompressionJobId,
    ) -> anyhow::Result<(CompressionJobStatus, Option<String>)> {
        let mut sleep_duration_sec: u32 = 1;
        loop {
            const POLLING_QUERY: &str = formatcp!(
                r"SELECT `status`, `status_msg` FROM `{table}` WHERE `id` = ?",
                table = CLP_COMPRESSION_JOB_TABLE_NAME
            );
            const MAX_SLEEP_DURATION_SEC: u32 = 30;

            let (status, status_msg) = sqlx::query_as::<_, (i32, Option<String>)>(POLLING_QUERY)
                .bind(id)
                .fetch_one(&self.db_pool)
                .await?;

            let status = CompressionJobStatus::try_from(status)
                .map_err(|_| anyhow::anyhow!("Invalid compression job status: {status}"))?;

            match status {
                CompressionJobStatus::Succeeded
                | CompressionJobStatus::Failed
                | CompressionJobStatus::Killed => {
                    return Ok((status, status_msg));
                }
                _ => {}
            }

            tokio::time::sleep(Duration::from_secs(sleep_duration_sec.into())).await;
            sleep_duration_sec =
                std::cmp::min(sleep_duration_sec.saturating_mul(2), MAX_SLEEP_DURATION_SEC);
        }
    }
}

/// Enum for CLP ingestion job status.
#[derive(
    Debug,
    Clone,
    Copy,
    PartialEq,
    Eq,
    sqlx::Encode,
    sqlx::Decode,
    EnumIter,
    AsRefStr,
    Display,
    EnumString,
)]
#[sqlx(rename_all = "snake_case")]
#[strum(serialize_all = "snake_case")]
pub enum ClpIngestionJobStatus {
    Requested,
    Running,
    Paused,
    Failed,
    Finished,
}

impl ClpIngestionJobStatus {
    /// Given the status transition, determines whether the transition is valid.
    ///
    /// # Returns
    ///
    /// Whether the transition (`from` -> `to`) is valid.
    const fn is_valid_transition(from: Self, to: Self) -> bool {
        match to {
            Self::Requested => false,
            Self::Failed => true,
            Self::Running => matches!(from, Self::Requested | Self::Paused),
            Self::Paused => matches!(from, Self::Running),
            Self::Finished => matches!(from, Self::Running | Self::Paused),
        }
    }
}

impl MySqlEnumFormat for ClpIngestionJobStatus {}

impl_sqlx_type!(ClpIngestionJobStatus => str);

/// Enum for CLP ingestion S3 object metadata status.
#[derive(
    Debug,
    Clone,
    Copy,
    PartialEq,
    Eq,
    sqlx::Encode,
    sqlx::Decode,
    EnumIter,
    AsRefStr,
    Display,
    EnumString,
)]
#[sqlx(rename_all = "snake_case")]
#[strum(serialize_all = "snake_case")]
pub enum IngestedS3ObjectMetadataStatus {
    Buffered,
    Submitted,
    Compressed,
    Failed,
}

impl MySqlEnumFormat for IngestedS3ObjectMetadataStatus {}

impl_sqlx_type!(IngestedS3ObjectMetadataStatus => str);

const INGESTION_JOB_TABLE_NAME: &str = "ingestion_job";
const INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME: &str = "ingestion_job_s3_scanner_state";
const INGESTED_S3_OBJECT_METADATA_TABLE_NAME: &str = "ingested_s3_object_metadata";

/// The query to create the table for CLP ingestion jobs.
#[must_use]
fn ingestion_job_table_creation_query() -> String {
    format!(
        r"
CREATE TABLE IF NOT EXISTS `{table}` (
    `id` BIGINT unsigned NOT NULL AUTO_INCREMENT,
    `config` TEXT NOT NULL,
    `status` {status_enum} NOT NULL DEFAULT '{default_status}',
    `status_msg` TEXT NULL DEFAULT NULL,
    `num_files_compressed` BIGINT unsigned NOT NULL DEFAULT '0',
    `creation_ts` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `last_update_ts` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
        ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (`id`)
);",
        table = INGESTION_JOB_TABLE_NAME,
        status_enum = ClpIngestionJobStatus::format_as_sql_enum(),
        default_status = ClpIngestionJobStatus::Requested,
    )
}

/// The query to create the table for S3 scanner job state tracking.
#[must_use]
fn ingestion_job_s3_scanner_state_table_creation_query() -> String {
    format!(
        r"
CREATE TABLE IF NOT EXISTS `{INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME}` (
    `id` BIGINT unsigned NOT NULL,
    `last_ingested_key` VARCHAR(1024) NULL DEFAULT NULL,
    PRIMARY KEY (`id`),
    CONSTRAINT `{INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME}_ingestion_job_id_ref`
        FOREIGN KEY (`id`) REFERENCES `{INGESTION_JOB_TABLE_NAME}` (`id`)
        ON DELETE RESTRICT ON UPDATE RESTRICT
);",
    )
}

/// The query to create the table for ingested S3 object metadata.
#[must_use]
fn ingested_s3_object_metadata_table_creation_query() -> String {
    format!(
        r"
CREATE TABLE IF NOT EXISTS `{table}` (
    `id` BIGINT unsigned NOT NULL AUTO_INCREMENT,
    `bucket` VARCHAR(1024) NOT NULL,
    `key` VARCHAR(1024) NOT NULL,
    `size` BIGINT unsigned NOT NULL,
    `status` {status_enum} NOT NULL DEFAULT '{default_status}',
    `ingestion_job_id` BIGINT unsigned NOT NULL,
    `compression_job_id` INT NULL DEFAULT NULL,
    `creation_ts` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `last_update_ts` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
        ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (`id`),
    INDEX `index_ingestion_job_id` (`ingestion_job_id`) USING BTREE,
    INDEX `index_compression_job_id` (`compression_job_id`) USING BTREE,
    CONSTRAINT `{table}_ingestion_job_id_ref`
        FOREIGN KEY (`ingestion_job_id`) REFERENCES `{job_table}` (`id`)
        ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT `{table}_compression_job_id_ref`
        FOREIGN KEY (`compression_job_id`) REFERENCES `{compression_job_table}` (`id`)
        ON DELETE RESTRICT ON UPDATE RESTRICT
);",
        table = INGESTED_S3_OBJECT_METADATA_TABLE_NAME,
        job_table = INGESTION_JOB_TABLE_NAME,
        status_enum = IngestedS3ObjectMetadataStatus::format_as_sql_enum(),
        default_status = IngestedS3ObjectMetadataStatus::Buffered,
        compression_job_table = crate::compression::CLP_COMPRESSION_JOB_TABLE_NAME,
    )
}

/// Updates the status of the underlying ingestion job in the CLP database.
///
/// # NOTE
///
/// This method is idempotent. It returns success if the underlying job is already in the target
/// status.
///
/// # Returns
///
/// `Ok(())` on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if the ingestion job is not found in the database.
/// * [`anyhow::Error`] if the status transition is invalid (as indicated by
///   [`ClpIngestionJobStatus::is_valid_transition`]).
/// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
/// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
/// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
async fn update_job_status(
    db_pool: MySqlPool,
    job_id: IngestionJobId,
    status: ClpIngestionJobStatus,
    status_msg: Option<&str>,
) -> anyhow::Result<()> {
    let mut tx = db_pool.begin().await?;

    // Lock the row for update and fetch the current status
    let curr_status: ClpIngestionJobStatus = sqlx::query_scalar(formatcp!(
        r"SELECT `status` FROM `{table}` WHERE `id` = ? FOR UPDATE",
        table = INGESTION_JOB_TABLE_NAME,
    ))
    .bind(job_id)
    .fetch_one(&mut *tx)
    .await
    .map_err(|e| match e {
        sqlx::Error::RowNotFound => {
            anyhow::anyhow!("Ingestion job with ID {job_id} not found.")
        }
        other => other.into(),
    })?;

    if curr_status == status {
        // For idempotency, we skip the update if the job is already in the target status.
        tx.commit().await?;
        return Ok(());
    }

    if !ClpIngestionJobStatus::is_valid_transition(curr_status, status) {
        const ERROR_MSG: &str = "Invalid job status transition.";
        tracing::error!(
            job_id = ? job_id,
            from_status = ? curr_status,
            to_status = ? status,
            ERROR_MSG
        );
        return Err(anyhow::anyhow!(ERROR_MSG));
    }

    sqlx::query(formatcp!(
        r"UPDATE `{table}` SET `status` = ?, `status_msg` = ? WHERE `id` = ?;",
        table = INGESTION_JOB_TABLE_NAME,
    ))
    .bind(status)
    .bind(status_msg)
    .bind(job_id)
    .execute(&mut *tx)
    .await?;

    tx.commit().await?;
    tracing::info!(job_id = ? job_id, status = ? status, "Ingestion job status updated.");
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_clp_ingestion_job_status_enum() {
        assert_eq!(
            ClpIngestionJobStatus::format_as_sql_enum(),
            "ENUM('requested', 'running', 'paused', 'failed', 'finished')"
        );
    }

    #[test]
    fn test_ingested_s3_object_metadata_status_enum() {
        assert_eq!(
            IngestedS3ObjectMetadataStatus::format_as_sql_enum(),
            "ENUM('buffered', 'submitted', 'compressed', 'failed')"
        );
    }
}
