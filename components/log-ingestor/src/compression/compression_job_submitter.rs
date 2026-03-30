use anyhow::Result;
use async_trait::async_trait;
use clp_rust_utils::{
    clp_config::{
        AwsAuthentication,
        S3Config,
        package::{DEFAULT_DATASET_NAME, config::ArchiveOutput},
    },
    job_config::{
        ClpIoConfig,
        CompressionJobId,
        CompressionJobStatus,
        InputConfig,
        OutputConfig,
        S3ObjectMetadataInputConfig,
        ingestion::s3::BaseConfig,
    },
    s3::S3ObjectMetadataId,
};

use crate::{compression::BufferSubmitter, ingestion_job_manager::ClpCompressionState};

/// The CLP compression job table name.
pub const CLP_COMPRESSION_JOB_TABLE_NAME: &str = "compression_jobs";

/// A compression submitter that implements [`BufferSubmitter`] to submit compression jobs to CLP.
pub struct CompressionJobSubmitter {
    io_config_template: ClpIoConfig,
    state: ClpCompressionState,
}

#[async_trait]
impl BufferSubmitter for CompressionJobSubmitter {
    async fn submit(&self, buffer: &[S3ObjectMetadataId]) -> Result<()> {
        let mut io_config = self.io_config_template.clone();
        match &mut io_config.input {
            InputConfig::S3ObjectMetadataInputConfig { config } => {
                config.s3_object_metadata_ids = buffer.to_vec();
            }
            InputConfig::S3InputConfig { .. } => {
                unreachable!("log-ingestor compression only supports `S3ObjectMetadataInputConfig`")
            }
        }
        let state = self.state.clone();
        tokio::spawn(submit_clp_compression_job_and_wait_for_completion(
            state, io_config,
        ));
        Ok(())
    }
}

impl CompressionJobSubmitter {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created instance with the given parameters, dedicated to submitting compression jobs
    /// using the given [`ClpCompressionState`] for its underlying ingestion job.
    #[must_use]
    pub fn new(
        clp_compression_state: ClpCompressionState,
        aws_authentication: AwsAuthentication,
        archive_output_config: &ArchiveOutput,
        ingestion_job_config: &BaseConfig,
    ) -> Self {
        let ingestion_job_id = clp_compression_state.get_ingestion_job_id();
        let s3_object_metadata_input_config = S3ObjectMetadataInputConfig {
            s3_config: S3Config {
                bucket: ingestion_job_config.bucket_name.clone(),
                region_code: ingestion_job_config.region.clone(),
                key_prefix: ingestion_job_config.key_prefix.clone(),
                endpoint_url: ingestion_job_config.endpoint_url.clone(),
                aws_authentication,
            },
            ingestion_job_id,
            s3_object_metadata_ids: vec![],
            // NOTE: Workaround for #1735
            dataset: Some(
                ingestion_job_config
                    .dataset
                    .clone()
                    .unwrap_or_else(|| DEFAULT_DATASET_NAME.clone()),
            ),
            timestamp_key: ingestion_job_config.timestamp_key.clone(),
            unstructured: ingestion_job_config.unstructured,
        };
        let output_config = OutputConfig {
            target_archive_size: archive_output_config.target_archive_size,
            target_dictionaries_size: archive_output_config.target_dictionaries_size,
            target_encoded_file_size: archive_output_config.target_encoded_file_size,
            target_segment_size: archive_output_config.target_segment_size,
            compression_level: archive_output_config.compression_level,
        };
        let io_config_template = ClpIoConfig {
            input: InputConfig::S3ObjectMetadataInputConfig {
                config: s3_object_metadata_input_config,
            },
            output: output_config,
        };
        Self {
            state: clp_compression_state,
            io_config_template,
        }
    }
}

/// Waits for the compression job to complete and updates the submitted metadata in the state
/// accordingly.
///
/// # NOTE
///
/// This function will be called inside a detached coroutine. Errors are logged only instead of
/// returning them to the caller.
pub async fn wait_for_compression_job_completion_and_update_metadata(
    state: ClpCompressionState,
    compression_job_id: CompressionJobId,
    num_objects_submitted: usize,
) {
    let ingestion_job_id = state.get_ingestion_job_id();
    match state
        .wait_for_compression_and_update_submitted_metadata(
            compression_job_id,
            num_objects_submitted,
        )
        .await
    {
        Ok((compression_job_status, message)) => match compression_job_status {
            CompressionJobStatus::Succeeded => tracing::info!(
                ingestion_job_id = ? ingestion_job_id,
                compression_job_id = ? compression_job_id,
                "Compression job succeeded."
            ),
            CompressionJobStatus::Failed | CompressionJobStatus::Killed => tracing::warn!(
                ingestion_job_id = ? ingestion_job_id,
                compression_job_id = ? compression_job_id,
                compression_job_status = ? compression_job_status,
                compression_job_status_msg = ? message,
                "Compression job failed."
            ),
            _ => unreachable!(
                "Unknown compression job status: {:?}",
                compression_job_status
            ),
        },
        Err(e) => {
            tracing::error!(
                ingestion_job_id = ? ingestion_job_id,
                compression_job_id = ? compression_job_id,
                error = ? e,
                "Failed to wait for CLP compression job completion."
            );
        }
    }
}

/// Submits a CLP compression job with the given IO config and waits for its completion.
///
/// # NOTE
///
/// This function logs errors instead of returning them to the caller since the caller is a detached
/// coroutine.
async fn submit_clp_compression_job_and_wait_for_completion(
    state: ClpCompressionState,
    io_config: ClpIoConfig,
) {
    let ingestion_job_id = state.get_ingestion_job_id();
    let num_objects_submitted = match &io_config.input {
        InputConfig::S3ObjectMetadataInputConfig { config } => config.s3_object_metadata_ids.len(),
        InputConfig::S3InputConfig { .. } => {
            unreachable!("log-ingestor compression only supports `S3ObjectMetadataInputConfig`")
        }
    };
    tracing::info!(ingestion_job_id = ? ingestion_job_id, "Submitting CLP compression job.");

    let compression_job_id = match state.submit_for_compression(io_config).await {
        Ok(id) => id,
        Err(e) => {
            tracing::error!(
                ingestion_job_id = ? ingestion_job_id,
                error = ? e,
                "Failed to submit CLP compression job."
            );
            return;
        }
    };
    tracing::info!(
        ingestion_job_id = ? ingestion_job_id,
        compression_job_id = ? compression_job_id,
        "Compression job submitted."
    );

    wait_for_compression_job_completion_and_update_metadata(
        state,
        compression_job_id,
        num_objects_submitted,
    )
    .await;
}
