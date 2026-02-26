use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::{
    clp_config::{AwsCredentials, package::config::ArchiveOutput},
    database::mysql::MySqlEnumFormat,
    impl_sqlx_type,
    job_config::{
        ClpIoConfig,
        CompressionJobId,
        CompressionJobStatus,
        InputConfig,
        ingestion::s3::S3IngestionJobConfig,
    },
    s3::{ObjectMetadata, S3ObjectMetadataId},
};
use const_format::formatcp;
use non_empty_string::NonEmptyString;
use sqlx::MySqlPool;
use strum_macros::{AsRefStr, Display, EnumIter, EnumString};
use tokio::sync::mpsc;

use crate::{
    compression::{Buffer, CLP_COMPRESSION_JOB_TABLE_NAME, CompressionJobSubmitter, Listener},
    ingestion_job::{IngestionJobState, S3ScannerState, SqsListenerState},
    ingestion_job_manager::IngestionJobId,
};

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
    /// Factory function.
    ///
    /// This method creates a new CLP DB connector and creates all necessary tables if they do not
    /// already exist.
    ///
    /// # Returns
    ///
    /// A newly created instance with the given database pool.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure if:
    ///   * [`INGESTION_JOB_TABLE_NAME`] table creation query execution fails.
    ///   * [`INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME`] table creation query execution fails.
    ///   * [`INGESTED_S3_OBJECT_METADATA_TABLE_NAME`] table creation query execution fails.
    pub async fn new(
        db_pool: MySqlPool,
        channel_capacity: usize,
        aws_credentials: AwsCredentials,
        archive_output_config: ArchiveOutput,
        buffer_flush_timeout: Duration,
        buffer_flush_threshold: u64,
    ) -> anyhow::Result<Self> {
        sqlx::query(ingestion_job_table_creation_query().as_str())
            .execute(&db_pool)
            .await?;

        sqlx::query(ingestion_job_s3_scanner_state_table_creation_query().as_str())
            .execute(&db_pool)
            .await?;

        sqlx::query(ingested_s3_object_metadata_table_creation_query().as_str())
            .execute(&db_pool)
            .await?;

        Ok(Self {
            db_pool,
            channel_capacity,
            aws_credentials,
            archive_output_config,
            buffer_flush_timeout,
            buffer_flush_threshold,
        })
    }

    /// Creates a new ingestion job in the CLP database with the given configuration.
    ///
    /// # Returns
    ///
    /// A tuple on success, containing:
    ///
    /// * A [`ClpIngestionState`] instance representing the initial state of the newly created
    ///   ingestion job.
    /// * A newly created listener for receiving ingested object metadata for compression job
    ///   submission.
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
        config: &S3IngestionJobConfig,
    ) -> anyhow::Result<(ClpIngestionState, Listener)> {
        const QUERY: &str = formatcp!(
            r"INSERT INTO `{table}` (`config`) VALUES (?);",
            table = INGESTION_JOB_TABLE_NAME,
        );

        let mut tx = self.db_pool.begin().await?;
        let job_id = sqlx::query(QUERY)
            .bind(serde_json::to_string(config)?)
            .execute(&mut *tx)
            .await?
            .last_insert_id();

        if let S3IngestionJobConfig::S3Scanner(_) = config {
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

        let submitter = CompressionJobSubmitter::new(
            ClpCompressionState {
                ingestion_job_id: job_id,
                db_pool: self.db_pool.clone(),
            },
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

        Ok((ingestion_state, listener))
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
        &self,
        status: ClpIngestionJobStatus,
        status_msg: Option<&str>,
    ) -> anyhow::Result<()> {
        let mut tx = self.db_pool.begin().await?;

        // Lock the row for update and fetch the current status
        let curr_status: ClpIngestionJobStatus = sqlx::query_scalar(formatcp!(
            r"SELECT `status` FROM `{table}` WHERE `id` = ? FOR UPDATE",
            table = INGESTION_JOB_TABLE_NAME,
        ))
        .bind(self.job_id)
        .fetch_one(&mut *tx)
        .await
        .map_err(|e| match e {
            sqlx::Error::RowNotFound => {
                anyhow::anyhow!("Ingestion job with ID {} not found.", self.job_id)
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
                job_id = ? self.job_id,
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
        .bind(self.job_id)
        .execute(&mut *tx)
        .await?;

        tx.commit().await?;
        tracing::info!(job_id = ? self.job_id, status = ? status, "Ingestion job status updated.");
        Ok(())
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
            r"SELECT `status` FROM `{table}` WHERE `id` = ?",
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
            let mut next_metadata_id = *last_inserted_ids.get(chunk_id).expect("invalid chunk ID");
            for object in chunk {
                object.id = Some(next_metadata_id);
                next_metadata_id += 1;
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
    /// * Forwards [`Self::update_job_status`]'s return values on failure.
    async fn start(&self) -> anyhow::Result<()> {
        self.update_job_status(ClpIngestionJobStatus::Running, None)
            .await
    }

    /// # NOTE
    ///
    /// This method is idempotent. It returns success if the underlying job is already in the
    /// terminated state.
    ///
    /// # Errors
    ///
    /// * Forwards [`Self::update_job_status`]'s return values on failure.
    async fn end(&self) -> anyhow::Result<()> {
        self.update_job_status(ClpIngestionJobStatus::Finished, None)
            .await
    }

    /// # NOTE
    ///
    /// This method is idempotent. It returns success if the underlying job is already in the failed
    /// state. The previous error message will not be overwritten.
    ///
    /// # Errors
    ///
    /// * Forwards [`Self::update_job_status`]'s return values on failure.
    async fn fail(&self, msg: String) {
        match self
            .update_job_status(ClpIngestionJobStatus::Failed, Some(&msg))
            .await
        {
            Ok(()) => {}
            Err(err) => {
                tracing::error!(
                    error = ? err,
                    status_msg = ? msg,
                    job_id = ? self.job_id,
                    "Failed to update job status to failed."
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
    `error_msg` TEXT NULL DEFAULT NULL,
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
