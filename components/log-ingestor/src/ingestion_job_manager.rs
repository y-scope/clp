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
    job_config::S3IngestionBaseConfig,
    s3::{ObjectMetadata, create_new_client as create_s3_client},
    sqs::create_new_client as create_sqs_client,
};
use tokio::sync::{Mutex, mpsc};
use uuid::Uuid;

use crate::{
    aws_client_manager::{S3ClientWrapper, SqsClientWrapper},
    compression::{Buffer, CompressionJobSubmitter, Listener},
    ingestion_job::{IngestionJob, S3Scanner, S3ScannerConfig, SqsListenerConfig},
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
}

/// An async-safe manager for creating and managing ingestion jobs.
pub struct IngestionJobManager {
    job_table: Arc<Mutex<HashMap<Uuid, IngestionJobTableEntry>>>,
    buffer_timeout: Duration,
    buffer_size_threshold: u64,
    channel_capacity: usize,
    aws_credentials: AwsCredentials,
    archive_output_config: ArchiveOutput,
    mysql_pool: sqlx::MySqlPool,
}

impl IngestionJobManager {
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
    /// * Forwards [`clp_rust_utils::database::mysql::create_mysql_pool`]'s return values on
    ///   failure.
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
        let mysql_pool = clp_rust_utils::database::mysql::create_mysql_pool(
            &clp_config.database,
            &clp_credentials.database,
            10,
        )
        .await?;
        Ok(Self {
            job_table: Arc::new(Mutex::new(HashMap::new())),
            buffer_timeout: Duration::from_secs(clp_config.log_ingestor.buffer_timeout_sec),
            buffer_size_threshold: clp_config.log_ingestor.buffer_size_threshold,
            channel_capacity: clp_config.log_ingestor.channel_capacity,
            aws_credentials,
            archive_output_config: clp_config.archive_output,
            mysql_pool,
        })
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
    /// * Forwards [`Self::create_s3_ingestion_job`]'s return values on failure.
    pub async fn create_s3_scanner_job(&self, config: S3ScannerConfig) -> Result<Uuid, Error> {
        let ingestion_job_config = config.base.clone();
        let s3_client_manager = self
            .create_s3_client_manager(config.base.region.as_str())
            .await;
        self.create_s3_ingestion_job(ingestion_job_config, move |job_id, sender| {
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
    pub async fn create_sqs_listener(&self, config: SqsListenerConfig) -> Result<Uuid, Error> {
        let ingestion_job_config = config.base.clone();
        let sqs_client_manager = self
            .create_sqs_client_manager(config.base.region.as_str())
            .await;
        self.create_s3_ingestion_job(ingestion_job_config, move |job_id, sender| {
            let listener = crate::ingestion_job::SqsListener::spawn(
                job_id,
                sqs_client_manager,
                config,
                sender,
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
        let mut job_table = self.job_table.lock().await;
        let job_to_remove = job_table.remove(&job_id);
        drop(job_table);

        match job_to_remove {
            Some(entry) => {
                entry.ingestion_job.shutdown_and_join().await?;
                entry.listener.shutdown_and_join().await?;
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
        ingestion_job_config: S3IngestionBaseConfig,
        create_ingestion_job: JobCreationCallback,
    ) -> Result<Uuid, Error>
    where
        JobCreationCallback: FnOnce(Uuid, mpsc::Sender<ObjectMetadata>) -> IngestionJob, {
        let mut job_table = self.job_table.lock().await;
        for table_entry in job_table.values() {
            if table_entry.region != ingestion_job_config.region
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

        let region = ingestion_job_config.region.clone();
        let bucket_name = ingestion_job_config.bucket_name.clone();
        let key_prefix = ingestion_job_config.key_prefix.clone();
        let dataset = ingestion_job_config.dataset.clone();
        // At this point, we use one listener per ingestion job. However, the listener itself is
        // designed to be shared among multiple ingestion jobs in the future.
        let job_listener = self.create_listener(ingestion_job_config);
        let sender = job_listener.get_new_sender();
        job_table.insert(
            job_id,
            IngestionJobTableEntry {
                ingestion_job: create_ingestion_job(job_id, sender),
                listener: job_listener,
                bucket_name,
                region,
                key_prefix,
                dataset,
            },
        );
        drop(job_table);

        Ok(job_id)
    }

    /// # Returns
    ///
    /// A new S3 client (wrapped by [`S3ClientWrapper`]) for the specified region.
    async fn create_s3_client_manager(&self, region: &str) -> S3ClientWrapper {
        let s3_endpoint = format!("https://s3.{region}.amazonaws.com");
        let s3_client = create_s3_client(
            s3_endpoint.as_str(),
            region,
            self.aws_credentials.access_key_id.as_str(),
            &self.aws_credentials.secret_access_key,
        )
        .await;
        S3ClientWrapper::from(s3_client)
    }

    /// # Returns
    ///
    /// A new SQS client (wrapped by [`SqsClientWrapper`]) for the specified region.
    async fn create_sqs_client_manager(&self, region: &str) -> SqsClientWrapper {
        let sqs_endpoint = format!("https://sqs.{region}.amazonaws.com");
        let sqs_client = create_sqs_client(
            sqs_endpoint.as_str(),
            region,
            self.aws_credentials.access_key_id.as_str(),
            &self.aws_credentials.secret_access_key,
        )
        .await;
        SqsClientWrapper::from(sqs_client)
    }

    /// # Returns
    ///
    /// A new listener for receiving object metadata to ingest.
    fn create_listener(&self, ingestion_job_config: S3IngestionBaseConfig) -> Listener {
        let submitter = CompressionJobSubmitter::new(
            self.mysql_pool.clone(),
            self.aws_credentials.clone(),
            &self.archive_output_config,
            ingestion_job_config,
        );
        Listener::spawn(
            Buffer::new(submitter, self.buffer_size_threshold),
            self.buffer_timeout,
            self.channel_capacity,
        )
    }
}

/// Represents an entry in the ingestion job table.
struct IngestionJobTableEntry {
    ingestion_job: IngestionJob,
    listener: Listener,
    region: String,
    bucket_name: String,
    key_prefix: String,
    dataset: Option<String>,
}

/// # Returns:
///
/// Whether the two strings are mutually prefix-free, meaning that `a` is not a prefix of `b` and
/// vice versa.
fn is_mutually_prefix_free(a: &str, b: &str) -> bool {
    !a.starts_with(b) && !b.starts_with(a)
}
