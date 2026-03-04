mod clp_ingestion;

use std::{collections::HashMap, sync::Arc};

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
use serde::Serialize;
use tokio::sync::Mutex;
use utoipa::ToSchema;

use crate::{
    aws_client_manager::{S3ClientWrapper, SqsClientWrapper},
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

/// Status of a terminated ingestion job.
#[derive(Clone, Serialize, ToSchema)]
#[serde(rename_all = "snake_case")]
pub enum TerminalStatus {
    Finished,
    Failed,
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
    /// * [`anyhow::Error`] if the logs input type in the CLP configuration is unsupported.
    /// * Forwards [`ClpDbIngestionConnector::connect`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if `clp_config.log_ingestor` is `None`.
    pub async fn from_config(
        clp_config: ClpConfig,
        clp_credentials: ClpCredentials,
    ) -> anyhow::Result<Self> {
        let aws_credentials = match &clp_config.logs_input {
            LogsInput::S3 { config } => match config.aws_authentication.clone() {
                AwsAuthentication::Credentials { credentials } => credentials,
            },
            LogsInput::Fs { .. } => {
                return Err(anyhow::anyhow!(
                    "Invalid CLP config: Unsupported logs input type. The current implementation \
                     only supports S3 input."
                ));
            }
        };

        let (clp_db_ingestion_connector, recovery_context) =
            ClpDbIngestionConnector::connect(clp_config, clp_credentials).await?;
        let inner = Arc::new(IngestionJobManager {
            job_table: Mutex::new(HashMap::new()),
            clp_db_ingestion_connector,
            aws_credentials,
        });
        let ingestion_job_manager = Self { inner };

        // Recover log-ingestor
        let (unfinished_compression_jobs, recoverable_ingestion_jobs, inactive_ingestion_jobs) = (
            recovery_context.unfinished_compression_jobs,
            recovery_context.recoverable_ingestion_jobs,
            recovery_context.inactive_ingestion_jobs,
        );
        try_recover_waiting_coroutines_for_unfinished_compression_jobs(unfinished_compression_jobs);
        try_recover_ingestion_job_instances(
            ingestion_job_manager.clone(),
            recoverable_ingestion_jobs,
        )
        .await;
        try_recover_inactive_ingestion_jobs(inactive_ingestion_jobs).await;

        Ok(ingestion_job_manager)
    }

    /// Registers a new S3 ingestion job with the given config in CLP DB and creates a running job
    /// instance in the underlying job table.
    ///
    /// # Returns
    ///
    /// The ID of the created ingestion job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::create_s3_ingestion_job_instance`]'s return values on failure.
    /// * Forwards [`ClpDbIngestionConnector::create_ingestion_job`]'s return values on failure.
    pub async fn register_and_create_s3_ingestion_job(
        &self,
        config: S3IngestionJobConfig,
    ) -> Result<IngestionJobId, Error> {
        let ingestion_job_context = self
            .inner
            .clp_db_ingestion_connector
            .create_ingestion_job(config)
            .await?;
        let ingestion_state = ingestion_job_context.get_ingestion_state();
        match self
            .create_s3_ingestion_job_instance(ingestion_job_context)
            .await
        {
            Ok(()) => Ok(ingestion_state.get_job_id()),
            Err(e) => {
                ingestion_state
                    .fail(format!("Failed to add S3 ingestion job instance: {e}"))
                    .await;
                Err(e)
            }
        }
    }

    /// Shuts down and removes an ingestion job instance by its ID.
    ///
    /// # NOTE
    ///
    /// This method only removes the running job instance. It does not remove the job from CLP DB.
    /// The job status and stats will still be accessible after calling this method.
    ///
    /// # Returns
    ///
    /// The terminal status of the job indicating whether it has stopped with an error.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::JobNotFound`] if the given job ID does not exist.
    /// * Forwards [`ClpIngestionJobContext::shutdown`]'s return values on failure.
    /// * Forwards [`ClpDbIngestionConnector::get_job_status`]' return values on failure.
    /// * Forwards [`ClpDbIngestionConnector::try_fail`]'s return values on failure.
    pub async fn shutdown_and_remove_job_instance(
        &self,
        job_id: IngestionJobId,
    ) -> Result<TerminalStatus, Error> {
        let mut job_table = self.inner.job_table.lock().await;
        let job_to_remove = job_table.remove(&job_id);
        drop(job_table);

        if let Some(entry) = job_to_remove {
            entry.ingestion_job_instance.shutdown_and_join().await;
            return entry
                .ingestion_job_context
                .shutdown()
                .await
                .map_err(Into::into);
        }

        // The ID does not exist in the in-memory job table.
        let Some(status) = self
            .inner
            .clp_db_ingestion_connector
            .get_job_status(job_id)
            .await?
        else {
            return Err(Error::JobNotFound(job_id));
        };

        if ClpIngestionJobStatus::Finished == status {
            // The job has already finished. Keep the operation idempotent by not overwriting the
            // status.
            Ok(TerminalStatus::Finished)
        } else {
            self.inner
                .clp_db_ingestion_connector
                .try_fail(
                    job_id,
                    "Ingestion job instance not found on shutdown.".to_string(),
                )
                .await?;
            Ok(TerminalStatus::Failed)
        }
    }

    /// Creates a new S3 ingestion job instance and adds it to the job table with key prefix
    /// conflict detection.
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
    /// * Forwards [`ValidatedSqsListenerConfig::validate_and_create`]'s return values on failure.
    /// * Forwards [`ClpIngestionState::start`]'s return values on failure.
    async fn create_s3_ingestion_job_instance(
        &self,
        ingestion_job_context: ClpIngestionJobContext,
    ) -> Result<(), Error> {
        let base_config = ingestion_job_context
            .get_ingestion_job_config()
            .as_base_config()
            .clone();
        let job_id = ingestion_job_context.get_job_id();
        let mut job_table = self.inner.job_table.lock().await;

        if job_table.contains_key(&job_id) {
            return Err(Error::InternalError(anyhow::anyhow!(
                "Job ID collision detected: An ingestion job with ID {job_id} already exists."
            )));
        }

        for table_entry in job_table.values() {
            let existing_job_base_config = table_entry
                .ingestion_job_context
                .get_ingestion_job_config()
                .as_base_config();
            // TODO: We should avoid being verbose for checking each field one by one (tracked by
            // #1805)
            if existing_job_base_config.endpoint_url != base_config.endpoint_url
                || existing_job_base_config.region != base_config.region
                || existing_job_base_config.bucket_name != base_config.bucket_name
                || existing_job_base_config.dataset != base_config.dataset
                || is_mutually_prefix_free(
                    existing_job_base_config.key_prefix.as_str(),
                    base_config.key_prefix.as_str(),
                )
            {
                continue;
            }
            return Err(Error::PrefixConflict(format!(
                "Cannot create ingestion job with prefix '{}' as it conflicts with existing job \
                 with prefix '{}', which ingests from the same region and bucket into the same \
                 dataset.",
                base_config.key_prefix, existing_job_base_config.key_prefix
            )));
        }

        let ingestion_state = ingestion_job_context.get_ingestion_state();
        let ingestion_job_instance = match ingestion_job_context.get_ingestion_job_config().clone()
        {
            S3IngestionJobConfig::SqsListener(config) => {
                let sqs_client_manager = SqsClientWrapper::create(
                    config.base.region.as_ref(),
                    self.inner.aws_credentials.access_key_id.as_str(),
                    self.inner.aws_credentials.secret_access_key.as_str(),
                )
                .await;
                let validated_config = ValidatedSqsListenerConfig::validate_and_create(config)?;
                ingestion_state.start().await?;
                IngestionJob::SqsListener(crate::ingestion_job::SqsListener::spawn(
                    job_id,
                    &sqs_client_manager,
                    &validated_config,
                    ingestion_state.clone(),
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
                ingestion_state.start().await?;
                IngestionJob::S3Scanner(S3Scanner::spawn(
                    job_id,
                    s3_client_manager,
                    config,
                    ingestion_state.clone(),
                ))
            }
        };

        job_table.insert(
            job_id,
            IngestionJobTableEntry {
                ingestion_job_instance,
                ingestion_job_context,
            },
        );

        drop(job_table);

        Ok(())
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
    ingestion_job_instance: IngestionJob<ClpIngestionState>,
    ingestion_job_context: ClpIngestionJobContext,
}

/// # Returns:
///
/// Whether the two strings are mutually prefix-free, meaning that `a` is not a prefix of `b` and
/// vice versa.
fn is_mutually_prefix_free(a: &str, b: &str) -> bool {
    !a.starts_with(b) && !b.starts_with(a)
}

/// Recovers coroutines to wait for the completion of unfinished compression jobs from the given
/// list of waitable compression job contexts.
fn try_recover_waiting_coroutines_for_unfinished_compression_jobs(
    unfinished_compression_jobs: Vec<ClpCompressionJobContext>,
) {
    for compression_job_context in unfinished_compression_jobs {
        let compression_job_id = compression_job_context.get_compression_job_id();
        tracing::info!(
            compression_job_id = ? compression_job_id,
            "Recovering a coroutine to wait for an unfinished compression job on ingestion job \
                recovery."
        );
        compression_job_context.detach_and_wait_for_completion_and_update_metadata();
    }
}

/// Recovers ingestion job instances from the given list of recoverable ingestion job contexts.
///
/// For each recoverable ingestion job, this function attempts to:
///
/// 1. Refill the ingestion buffer with the buffered objects previously ingested.
/// 2. Create a running ingestion job instance and adds it to the job table.
///
/// # NOTE
///
/// This method is best-effort. If an error happens, it will be logged and the recovery process for
/// the given ingestion job will be skipped.
async fn try_recover_ingestion_job_instances(
    ingestion_job_manager: IngestionJobManagerState,
    recoverable_ingestion_jobs: Vec<ClpIngestionJobContext>,
) {
    let mut num_recovered_jobs = 0;
    for ingestion_job_context in recoverable_ingestion_jobs {
        let job_id = ingestion_job_context.get_job_id();
        tracing::info!(
            job_id = ? job_id,
            "Recovering ingestion job."
        );

        try_refill_ingestion_buffer(&ingestion_job_context).await;

        tracing::info!(
            job_id = ? job_id,
            "Creating ingestion job instance on recovery."
        );
        if let Err(e) = ingestion_job_manager
            .create_s3_ingestion_job_instance(ingestion_job_context)
            .await
        {
            tracing::error!(
                job_id = ? job_id,
                error = ? e,
                "Failed to create ingestion job instance on recovery. Recovery for this job \
                    skipped."
            );
        } else {
            num_recovered_jobs += 1;
        }
    }
    tracing::info!("Ingestion job recovery completed. Total recovered jobs: {num_recovered_jobs}.");
}

/// Recovers inactive ingestion jobs from the given list of ingestion job context.
///
/// For inactive compression jobs, this function only refills their ingestion buffers. Once this
/// function returns, the ingestion contexts are dropped, which closes the underlying compression
/// listeners. Any remaining buffered objects will then be submitted for compression.
///
/// # NOTE
///
/// This method is best-effort. If an error happens, it will be logged and the recovery process for
/// the given ingestion job will be skipped.
async fn try_recover_inactive_ingestion_jobs(inactive_ingestion_jobs: Vec<ClpIngestionJobContext>) {
    for ingestion_job_context in inactive_ingestion_jobs {
        try_refill_ingestion_buffer(&ingestion_job_context).await;
    }
}

/// Recovers the ingestion buffer by sending the previously buffered objects to the ingestion buffer
/// sender for compression.
///
/// # NOTE
///
/// This method is best-effort. If an error happens, it will be logged and the recovery process for
/// the given ingestion job will be skipped.
async fn try_refill_ingestion_buffer(ingestion_job_context: &ClpIngestionJobContext) {
    let job_id = ingestion_job_context.get_job_id();
    tracing::info!(
        job_id = ? job_id,
        "Refilling ingestion buffer with previously buffered objects on recovery."
    );

    let ingestion_job_state = ingestion_job_context.get_ingestion_state();
    let buffered_objects = match ingestion_job_state.get_buffered_object_metadata().await {
        Ok(buffered_objects) => buffered_objects,
        Err(e) => {
            tracing::error!(
                job_id = ? job_id,
                error = ? e,
                "Failed to get buffered objects on ingestion job recovery. Recovering skipped."
            );
            return;
        }
    };
    tracing::info!(
        job_id = ? job_id,
        num_buffered_objects = ? buffered_objects.len(),
        "Sending buffered objects to ingestion buffer on recovery."
    );
    if let Err(e) = ingestion_job_context
        .get_ingestion_buffer_sender()
        .send(buffered_objects)
        .await
    {
        tracing::error!(
            job_id = ? job_id,
            error = ? e,
            "Failed to send buffered objects to ingestion buffer on recovery. Recovering skipped."
        );
    }
}
