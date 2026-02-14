use std::{collections::HashMap, sync::Arc, time::Duration};

use clp_rust_utils::{
    clp_config::{
        AwsAuthentication,
        AwsCredentials,
        package::{
            config::{ArchiveOutput, Config as ClpConfig, LogsInput},
            credentials::Credentials as ClpCredentials,
        },
    },
    job_config::ingestion::s3::{
        BaseConfig,
        ConfigError,
        S3ScannerConfig,
        SqsListenerConfig,
        ValidatedSqsListenerConfig,
    },
    s3::ObjectMetadata,
};
use non_empty_string::NonEmptyString;
use tokio::sync::{Mutex, mpsc};
use uuid::Uuid;

use crate::{
    aws_client_manager::{S3ClientWrapper, SqsClientWrapper},
    compression::{Buffer, CompressionJobSubmitter, Listener},
    ingestion_job::{IngestionJob, S3Scanner},
};

/// Errors for ingestion job manager operations.
#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[error("Log ingestor internal error: {0}")]
    InternalError(#[from] anyhow::Error),

    #[error("Ingestion job not found: {0}")]
    JobNotFound(Uuid),

    #[error("Prefix conflict: {0}")]
    PrefixConflict(String),

    #[error("Custom endpoint URL not supported: {0}")]
    CustomEndpointUrlNotSupported(String),

    #[error("Invalid job config: {0}")]
    InvalidConfig(#[from] ConfigError),

    #[error("A region code must be specified when using the default AWS endpoint")]
    MissingRegionCode,
}

/// An async-safe state for creating and managing ingestion jobs.
#[derive(Clone)]
pub struct IngestionJobManagerState {
    inner: Arc<IngestionJobManager>,
}

impl IngestionJobManagerState {
    /// Factory function.
    ///
    /// Creates a new ingestion job manager from the given CLP configuration and credentials.
    ///
    /// # Returns
    ///
    /// A newly created ingestion job manager on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * The logs input type in the CLP configuration is unsupported.
    /// * Forwards [`clp_rust_utils::database::mysql::create_clp_db_mysql_pool`]'s return values on
    ///   failure.
    ///
    /// # Panics
    ///
    /// Panics if `clp_config.log_ingestor` is `None`.
    pub async fn from_config(
        clp_config: ClpConfig,
        clp_credentials: ClpCredentials,
    ) -> anyhow::Result<Self> {
        let aws_credentials = match clp_config.logs_input {
            LogsInput::S3 { config } => match config.aws_authentication {
                AwsAuthentication::Credentials { credentials } => credentials,
            },
            LogsInput::Fs { .. } => {
                return Err(anyhow::anyhow!(
                    "Invalid CLP config: Unsupported logs input type. The current implementation \
                     only supports S3 input."
                ));
            }
        };
        let mysql_pool = clp_rust_utils::database::mysql::create_clp_db_mysql_pool(
            &clp_config.database,
            &clp_credentials.database,
            10,
        )
        .await?;
        let log_ingestor_config = clp_config
            .log_ingestor
            .as_ref()
            .expect("log_ingestor configuration is missing");
        let inner = Arc::new(IngestionJobManager {
            job_table: Mutex::new(HashMap::new()),
            buffer_flush_timeout: Duration::from_secs(log_ingestor_config.buffer_flush_timeout_sec),
            buffer_flush_threshold: log_ingestor_config.buffer_flush_threshold,
            channel_capacity: log_ingestor_config.channel_capacity,
            aws_credentials,
            archive_output_config: clp_config.archive_output,
            mysql_pool,
        });
        Ok(Self { inner })
    }

    /// Creates a new S3 scanner ingestion job.
    ///
    /// # Returns
    ///
    /// `Ok(Uuid)` containing the job ID on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::MissingRegionCode`] if both the endpoint URL and region are unspecified.
    /// * Forwards [`Self::create_s3_ingestion_job`]'s return values on failure.
    pub async fn create_s3_scanner_job(&self, config: S3ScannerConfig) -> Result<Uuid, Error> {
        if config.base.endpoint_url.is_none() && config.base.region.is_none() {
            return Err(Error::MissingRegionCode);
        }
        let s3_client_manager = S3ClientWrapper::create(
            config.base.region.as_ref(),
            self.inner.aws_credentials.access_key_id.as_str(),
            self.inner.aws_credentials.secret_access_key.as_str(),
            config.base.endpoint_url.as_ref(),
        )
        .await;
        self.create_s3_ingestion_job(config.base.clone(), move |job_id, sender| {
            let scanner = S3Scanner::spawn(job_id, s3_client_manager, config, sender);
            IngestionJob::S3Scanner(scanner)
        })
        .await
    }

    /// Creates a new SQS listener ingestion job.
    ///
    /// # Returns
    ///
    /// `Ok(Uuid)` containing the job ID on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::create_s3_ingestion_job`]'s return values on failure.
    /// * Forwards [`ValidatedSqsListenerConfig::validate_and_create`]'s return values on failure.
    pub async fn create_sqs_listener_job(
        &self,
        raw_config: SqsListenerConfig,
    ) -> Result<Uuid, Error> {
        let config = ValidatedSqsListenerConfig::validate_and_create(raw_config)?;
        let ingestion_job_config = config.get().base.clone();
        if let Some(endpoint_url) = &ingestion_job_config.endpoint_url {
            return Err(Error::CustomEndpointUrlNotSupported(format!(
                "SQS listener ingestion jobs do not support custom endpoint URLs yet. Endpoint \
                 URL: {endpoint_url}"
            )));
        }
        let sqs_client_manager = SqsClientWrapper::create(
            config.get().base.region.as_ref(),
            self.inner.aws_credentials.access_key_id.as_str(),
            self.inner.aws_credentials.secret_access_key.as_str(),
        )
        .await;
        self.create_s3_ingestion_job(ingestion_job_config, move |job_id, sender| {
            let listener = crate::ingestion_job::SqsListener::spawn(
                job_id,
                &sqs_client_manager,
                &config,
                &sender,
            );
            IngestionJob::SqsListener(listener)
        })
        .await
    }

    /// Shuts down and removes an ingestion job by its ID.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::JobNotFound`] if the given job ID does not exist.
    /// * Forwards [`IngestionJob::shutdown_and_join`]'s return value on failure.
    /// * Forwards [`Listener::shutdown_and_join`]'s return value on failure.
    pub async fn shutdown_and_remove_job(&self, job_id: Uuid) -> Result<(), Error> {
        let mut job_table = self.inner.job_table.lock().await;
        let job_to_remove = job_table.remove(&job_id);
        drop(job_table);

        match job_to_remove {
            Some(entry) => {
                entry.ingestion_job.shutdown_and_join().await?;
                tracing::debug!("Ingestion job {} shut down.", job_id);
                entry.listener.shutdown_and_join().await?;
                tracing::debug!("Ingestion job {}'s listener shut down.", job_id);
                Ok(())
            }
            None => Err(Error::JobNotFound(job_id)),
        }
    }

    /// Creates a new S3 ingestion job with key prefix conflict detection.
    ///
    /// When multiple ingestion jobs run in parallel, this function ensures that key prefixes do not
    /// conflict with each other, preventing duplicate object ingestion.
    ///
    /// A conflict is detected when all the following conditions are met:
    ///
    /// * The regions of an existing job and the new job are identical, and
    /// * The bucket names of an existing job and the new job are identical, and
    /// * The datasets of an existing job and the new job are identical, and
    /// * The key prefixes are not mutually prefix-free (i.e., one is a prefix of the other).
    ///
    /// # Type Parameters
    ///
    /// * `JobCreationCallback` - A callback function type that the caller implements to create the
    ///   ingestion job with the desired type.
    ///
    /// # Returns
    ///
    /// `Ok(Uuid)` containing the job ID on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::PrefixConflict`] if the given key prefix conflicts with an existing job's prefix.
    async fn create_s3_ingestion_job<JobCreationCallback>(
        &self,
        ingestion_job_config: BaseConfig,
        create_ingestion_job: JobCreationCallback,
    ) -> Result<Uuid, Error>
    where
        JobCreationCallback: FnOnce(Uuid, mpsc::Sender<ObjectMetadata>) -> IngestionJob, {
        let mut job_table = self.inner.job_table.lock().await;
        for table_entry in job_table.values() {
            // TODO: We should avoid being verbose for checking each field one by one (tracked by
            // #1805)
            if table_entry.endpoint_url != ingestion_job_config.endpoint_url
                || table_entry.region != ingestion_job_config.region
                || table_entry.bucket_name != ingestion_job_config.bucket_name
                || table_entry.dataset != ingestion_job_config.dataset
                || is_mutually_prefix_free(
                    table_entry.key_prefix.as_str(),
                    ingestion_job_config.key_prefix.as_str(),
                )
            {
                continue;
            }
            return Err(Error::PrefixConflict(format!(
                "Cannot create ingestion job with prefix '{}' as it conflicts with existing job \
                 with prefix '{}', which ingests from the same region and bucket into the same \
                 dataset.",
                ingestion_job_config.key_prefix, table_entry.key_prefix
            )));
        }

        let job_id = {
            loop {
                let candidate_job_id = Uuid::new_v4();
                if !job_table.contains_key(&candidate_job_id) {
                    break candidate_job_id;
                }
            }
        };

        // At this point, we use one listener per ingestion job. However, the listener itself is
        // designed to be shared among multiple ingestion jobs in the future.
        let job_listener = self.create_listener(&ingestion_job_config);
        let sender = job_listener.get_new_sender();
        job_table.insert(
            job_id,
            IngestionJobTableEntry {
                ingestion_job: create_ingestion_job(job_id, sender),
                listener: job_listener,
                bucket_name: ingestion_job_config.bucket_name,
                region: ingestion_job_config.region,
                key_prefix: ingestion_job_config.key_prefix,
                endpoint_url: ingestion_job_config.endpoint_url,
                dataset: ingestion_job_config.dataset,
            },
        );
        drop(job_table);

        Ok(job_id)
    }

    /// # Returns
    ///
    /// A new listener for receiving object metadata to ingest.
    fn create_listener(&self, ingestion_job_config: &BaseConfig) -> Listener {
        let submitter = CompressionJobSubmitter::new(
            self.inner.mysql_pool.clone(),
            self.inner.aws_credentials.clone(),
            &self.inner.archive_output_config,
            ingestion_job_config,
        );
        Listener::spawn(
            Buffer::new(submitter, self.inner.buffer_flush_threshold),
            self.inner.buffer_flush_timeout,
            self.inner.channel_capacity,
        )
    }
}

/// Internal shared states for managing ingestion jobs. Must be wrapped in an `Arc`.
struct IngestionJobManager {
    job_table: Mutex<HashMap<Uuid, IngestionJobTableEntry>>,
    buffer_flush_timeout: Duration,
    buffer_flush_threshold: u64,
    channel_capacity: usize,
    aws_credentials: AwsCredentials,
    archive_output_config: ArchiveOutput,
    mysql_pool: sqlx::MySqlPool,
}

/// Represents an entry in the ingestion job table.
struct IngestionJobTableEntry {
    ingestion_job: IngestionJob,
    listener: Listener,
    region: Option<NonEmptyString>,
    bucket_name: NonEmptyString,
    key_prefix: NonEmptyString,
    endpoint_url: Option<NonEmptyString>,
    dataset: Option<NonEmptyString>,
}

/// # Returns:
///
/// Whether the two strings are mutually prefix-free, meaning that `a` is not a prefix of `b` and
/// vice versa.
fn is_mutually_prefix_free(a: &str, b: &str) -> bool {
    !a.starts_with(b) && !b.starts_with(a)
}
