//! Handle for driving a single S3 compression job to completion.

use std::{sync::Arc, time::Duration};

use clp_rust_utils::{
    clp_config::package::config::Database,
    dataset::VALID_DATASET_NAME_REGEX,
    job_config::{ClpIoConfig, CompressionJobId, CompressionJobStatus, InputConfig},
    task_io::compression::{ClpSCompressionOption, S3InputSource},
};
use spider_core::{
    task::ExecutionPolicy,
    types::id::{JobId as SpiderJobId, ResourceGroupId},
};
use sqlx::MySqlPool;

use crate::{Error, compression_job_submitter::S3CompressionJobSubmitter};

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
    _db_pool: MySqlPool,
    _db_config: Database,
    compression_job_id: CompressionJobId,
    job_submitter: SubmitterType,
    resource_group_id: ResourceGroupId,

    _input_config: InputConfig,
    clp_s_compression_option: ClpSCompressionOption,
    dataset: Option<String>,
    _target_archive_size: u64,

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
            _db_pool: db_pool,
            _db_config: db_config,
            compression_job_id,
            job_submitter,
            resource_group_id,
            _input_config: input_config,
            clp_s_compression_option,
            dataset,
            _target_archive_size: output_config.target_archive_size,
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

        self.persist_spider_job_id(spider_job_id).await?;
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
    /// partitions the objects into compression task inputs, and derives an execution policy for
    /// each task based on the number of objects it contains.
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
    /// TODO
    async fn prepare_task_inputs(&self) -> Result<Vec<(S3InputSource, ExecutionPolicy)>, Error> {
        todo!("implement me!")
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
    /// TODO
    async fn upsert_metadata_tables(&self) -> Result<(), Error> {
        todo!("implement me!")
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
    /// TODO
    async fn persist_spider_job_id(&self, _spider_job_id: SpiderJobId) -> Result<(), Error> {
        todo!("implement me!")
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
    /// TODO
    async fn to_completion(&self, _spider_job_id: SpiderJobId) -> Result<(), Error> {
        todo!("implement me!")
    }

    /// Updates the compression job status in the CLP database.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// TODO
    async fn update_job_status(
        &self,
        _job_status: CompressionJobStatus,
        _status_message: Option<String>,
    ) -> Result<(), Error> {
        todo!("implement me!")
    }
}
