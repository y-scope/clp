mod clp_ingestion;

use std::{collections::HashMap, sync::Arc, time::Duration};

pub use clp_ingestion::*;
use clp_rust_utils::{
    clp_config::{
        AwsAuthentication,
        AwsCredentials,
        package::{
            config::{Config as ClpConfig, LogsInput},
            credentials::Credentials as ClpCredentials,
        },
    },
    job_config::ingestion::s3::{ConfigError, S3IngestionJobConfig, ValidatedSqsListenerConfig},
};
use non_empty_string::NonEmptyString;
use tokio::sync::Mutex;

use crate::{
    aws_client_manager::{S3ClientWrapper, SqsClientWrapper},
    compression::Listener,
    ingestion_job::{IngestionJob, IngestionJobId, IngestionJobState, S3Scanner},
};

/// Errors for ingestion job manager operations.
#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[error("Log ingestor internal error: {0}")]
    InternalError(#[from] anyhow::Error),

    #[error("Ingestion job not found: {0}")]
    JobNotFound(IngestionJobId),

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
            100,
        )
        .await?;
        let log_ingestor_config = clp_config
            .log_ingestor
            .as_ref()
            .expect("log_ingestor configuration is missing");
        let inner = Arc::new(IngestionJobManager {
            job_table: Mutex::new(HashMap::new()),
            clp_db_ingestion_connector: ClpDbIngestionConnector::new(
                mysql_pool,
                log_ingestor_config.channel_capacity,
                aws_credentials.clone(),
                clp_config.archive_output,
                Duration::from_secs(log_ingestor_config.buffer_flush_timeout_sec),
                log_ingestor_config.buffer_flush_threshold,
            )
            .await?,
            aws_credentials,
        });
        Ok(Self { inner })
    }

    /// Creates a new S3 ingestion job based on the given config, and adds a running job instance to
    /// the underlying job table.
    ///
    /// # Returns
    ///
    /// The ID of the created ingestion job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::add_s3_ingestion_job`]'s return values on failure.
    /// * Forwards [`ClpDbIngestionConnector::create_ingestion_job`]'s return values on failure.
    pub async fn create_new_s3_ingestion_job(
        &self,
        config: S3IngestionJobConfig,
    ) -> Result<IngestionJobId, Error> {
        let (state, listener) = self
            .inner
            .clp_db_ingestion_connector
            .create_ingestion_job(&config)
            .await?;
        match self
            .add_s3_ingestion_job(config, state.clone(), listener)
            .await
        {
            Ok(()) => Ok(state.get_job_id()),
            Err(e) => {
                state
                    .fail(format!("Failed to add S3 ingestion job instance: {e}"))
                    .await;
                Err(e)
            }
        }
    }

    /// Shuts down and removes an ingestion job instance by its ID.
    ///
    /// # Returns
    ///
    /// A boolean indicating whether the job has stopped with an error:
    ///
    /// * `true` indicates the job ends in [`ClpIngestionJobStatus::Failed`] status.
    /// * `false` indicates the job ends in [`ClpIngestionJobStatus::Finished`] status.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::JobNotFound`] if the given job ID does not exist.
    /// * Forwards [`ClpIngestionState::get_job_status`]'s return values on failure.
    /// * Forwards [`IngestionJobState::end`]'s return values on failure.
    pub async fn shutdown_and_remove_job_instance(
        &self,
        job_id: IngestionJobId,
    ) -> Result<bool, Error> {
        let mut job_table = self.inner.job_table.lock().await;
        let job_to_remove = job_table.remove(&job_id);
        drop(job_table);

        match job_to_remove {
            Some(entry) => {
                entry.ingestion_job.shutdown_and_join().await;
                entry.listener.shutdown_and_join().await;
                if ClpIngestionJobStatus::Failed == entry.state.get_job_status().await? {
                    // The job has failed. We don't overwrite the failure status.
                    return Ok(true);
                }
                entry.state.end().await?;
                Ok(false)
            }
            None => Err(Error::JobNotFound(job_id)),
        }
    }

    /// Adds a new S3 ingestion job to the job table with key prefix conflict detection. The added
    /// ingestion job will start running immediately after being added to the job table.
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
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::InternalError`] if a job ID collision is detected (which is unlikely in
    ///   practice).
    /// * [`Error::PrefixConflict`] if the given key prefix conflicts with an existing job's prefix.
    /// * Forwards [`Self::start_s3_ingestion_job_instance`]'s return values on failure.
    async fn add_s3_ingestion_job(
        &self,
        config: S3IngestionJobConfig,
        state: ClpIngestionState,
        listener: Listener,
    ) -> Result<(), Error> {
        let base_config = config.as_base_config().clone();
        let job_id = state.get_job_id();
        let mut job_table = self.inner.job_table.lock().await;

        if job_table.contains_key(&job_id) {
            return Err(Error::InternalError(anyhow::anyhow!(
                "Job ID collision detected: An ingestion job with ID {job_id} already exists."
            )));
        }

        for table_entry in job_table.values() {
            // TODO: We should avoid being verbose for checking each field one by one (tracked by
            // #1805)
            if table_entry.endpoint_url != base_config.endpoint_url
                || table_entry.region != base_config.region
                || table_entry.bucket_name != base_config.bucket_name
                || table_entry.dataset != base_config.dataset
                || is_mutually_prefix_free(
                    table_entry.key_prefix.as_str(),
                    base_config.key_prefix.as_str(),
                )
            {
                continue;
            }
            return Err(Error::PrefixConflict(format!(
                "Cannot create ingestion job with prefix '{}' as it conflicts with existing job \
                 with prefix '{}', which ingests from the same region and bucket into the same \
                 dataset.",
                base_config.key_prefix, table_entry.key_prefix
            )));
        }

        job_table.insert(
            job_id,
            IngestionJobTableEntry {
                ingestion_job: self
                    .start_s3_ingestion_job_instance(config, state.clone())
                    .await?,
                listener,
                bucket_name: base_config.bucket_name,
                region: base_config.region,
                key_prefix: base_config.key_prefix,
                endpoint_url: base_config.endpoint_url,
                dataset: base_config.dataset,
                state,
            },
        );

        drop(job_table);

        Ok(())
    }

    /// Creates and starts a new S3 ingestion job instance based on the given config and state.
    ///
    /// # Returns
    ///
    /// The created ingestion job instance on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`ClpIngestionState::start`]'s return values on failure.
    /// * Forwards [`ValidatedSqsListenerConfig::validate_and_create`]'s return values on failure.
    async fn start_s3_ingestion_job_instance(
        &self,
        config: S3IngestionJobConfig,
        state: ClpIngestionState,
    ) -> Result<IngestionJob<ClpIngestionState>, Error> {
        let job_id = state.get_job_id();
        match config {
            S3IngestionJobConfig::SqsListener(config) => {
                let sqs_client_manager = SqsClientWrapper::create(
                    config.base.region.as_ref(),
                    self.inner.aws_credentials.access_key_id.as_str(),
                    self.inner.aws_credentials.secret_access_key.as_str(),
                )
                .await;
                let validated_config = ValidatedSqsListenerConfig::validate_and_create(config)?;
                state.start().await?;
                Ok(IngestionJob::SqsListener(
                    crate::ingestion_job::SqsListener::spawn(
                        job_id,
                        &sqs_client_manager,
                        &validated_config,
                        state.clone(),
                    ),
                ))
            }
            S3IngestionJobConfig::S3Scanner(config) => {
                let s3_client_manager = S3ClientWrapper::create(
                    config.base.region.as_ref(),
                    self.inner.aws_credentials.access_key_id.as_str(),
                    self.inner.aws_credentials.secret_access_key.as_str(),
                    config.base.endpoint_url.as_ref(),
                )
                .await;
                state.start().await?;
                Ok(IngestionJob::S3Scanner(S3Scanner::spawn(
                    job_id,
                    s3_client_manager,
                    config,
                    state.clone(),
                )))
            }
        }
    }
}

/// Internal shared states for managing ingestion jobs. Must be wrapped in an `Arc`.
struct IngestionJobManager {
    job_table: Mutex<HashMap<IngestionJobId, IngestionJobTableEntry>>,
    clp_db_ingestion_connector: ClpDbIngestionConnector,
    aws_credentials: AwsCredentials,
}

/// Represents an entry in the ingestion job table.
struct IngestionJobTableEntry {
    ingestion_job: IngestionJob<ClpIngestionState>,
    listener: Listener,
    region: Option<NonEmptyString>,
    bucket_name: NonEmptyString,
    key_prefix: NonEmptyString,
    endpoint_url: Option<NonEmptyString>,
    dataset: Option<NonEmptyString>,
    state: ClpIngestionState,
}

/// # Returns:
///
/// Whether the two strings are mutually prefix-free, meaning that `a` is not a prefix of `b` and
/// vice versa.
fn is_mutually_prefix_free(a: &str, b: &str) -> bool {
    !a.starts_with(b) && !b.starts_with(a)
}
