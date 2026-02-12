use anyhow::Result;
use async_trait::async_trait;
use clp_rust_utils::{
    clp_config::{
        AwsAuthentication::Credentials,
        AwsCredentials,
        S3Config,
        package::{DEFAULT_DATASET_NAME, config::ArchiveOutput},
    },
    job_config::{
        ClpIoConfig,
        CompressionJobStatus,
        InputConfig,
        OutputConfig,
        S3InputConfig,
        ingestion::s3::BaseConfig,
    },
    s3::ObjectMetadata,
};
use const_format::formatcp;
use sqlx::MySqlPool;

use crate::compression::BufferSubmitter;

/// The CLP compression job table name.
const CLP_COMPRESSION_JOB_TABLE_NAME: &str = "compression_jobs";

/// A compression submitter that implements [`BufferSubmitter`] to submit compression jobs to CLP.
pub struct CompressionJobSubmitter {
    db_pool: MySqlPool,
    io_config_template: ClpIoConfig,
}

#[async_trait]
impl BufferSubmitter for CompressionJobSubmitter {
    async fn submit(&self, buffer: &[ObjectMetadata]) -> Result<()> {
        let mut io_config = self.io_config_template.clone();
        let s3_input_config = match &mut io_config.input {
            InputConfig::S3InputConfig { config } => config,
        };
        s3_input_config.keys = Some(buffer.iter().map(|obj| obj.key.clone()).collect());
        tokio::spawn(submit_clp_compression_job_and_wait_for_completion(
            self.db_pool.clone(),
            io_config,
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
    /// created by the given ingestion job specification.
    #[must_use]
    pub fn new(
        db_pool: MySqlPool,
        aws_credentials: AwsCredentials,
        archive_output_config: &ArchiveOutput,
        ingestion_job_config: &BaseConfig,
    ) -> Self {
        let s3_input_config = S3InputConfig {
            s3_config: S3Config {
                bucket: ingestion_job_config.bucket_name.clone(),
                region_code: ingestion_job_config.region.clone(),
                key_prefix: ingestion_job_config.key_prefix.clone(),
                endpoint_url: ingestion_job_config.endpoint_url.clone(),
                aws_authentication: Credentials {
                    credentials: aws_credentials,
                },
            },
            keys: None,
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
            input: InputConfig::S3InputConfig {
                config: s3_input_config,
            },
            output: output_config,
        };
        Self {
            db_pool,
            io_config_template,
        }
    }
}

/// Submits a CLP compression job with the given IO config and waits for its completion.
///
/// # NOTE
///
/// * This function logs errors instead of returning them to the caller.
/// * This function interacts with CLP database directly. We should replace this with a CLP client
///   implementation once it's available.
async fn submit_clp_compression_job_and_wait_for_completion(
    db_pool: MySqlPool,
    io_config: ClpIoConfig,
) {
    const SUBMISSION_QUERY: &str = formatcp!(
        "INSERT INTO {} (`clp_config`) VALUES (?)",
        CLP_COMPRESSION_JOB_TABLE_NAME
    );
    const POLLING_QUERY: &str =
        formatcp!("SELECT status, status_msg FROM {CLP_COMPRESSION_JOB_TABLE_NAME} WHERE id = ?");
    const MAX_SLEEP_DURATION_SEC: u32 = 30;

    let serialized_io_config = match clp_rust_utils::serde::BrotliMsgpack::serialize(&io_config) {
        Ok(data) => data,
        Err(e) => {
            tracing::error!("Failed to serialize CLP IO config: {}", e);
            return;
        }
    };

    tracing::info!("Submitting CLP compression job...");
    let job_id = match sqlx::query(SUBMISSION_QUERY)
        .bind(serialized_io_config)
        .execute(&db_pool)
        .await
    {
        Ok(result) => result.last_insert_id(),
        Err(e) => {
            tracing::error!("Failed to submit CLP compression job: {}", e);
            return;
        }
    };
    tracing::info!("Compression job submitted with ID: {}", job_id);

    let mut sleep_duration_sec: u32 = 1;
    loop {
        tokio::time::sleep(tokio::time::Duration::from_secs(sleep_duration_sec.into())).await;
        sleep_duration_sec =
            std::cmp::min(sleep_duration_sec.saturating_mul(2), MAX_SLEEP_DURATION_SEC);

        match sqlx::query_as::<_, (i32, Option<String>)>(POLLING_QUERY)
            .bind(job_id)
            .fetch_one(&db_pool)
            .await
        {
            Ok((status, status_message)) => {
                let status = match CompressionJobStatus::try_from(status) {
                    Ok(s) => s,
                    Err(e) => {
                        tracing::error!("Unknown compression job status code {}: {}", status, e);
                        return;
                    }
                };
                match status {
                    CompressionJobStatus::Failed => {
                        tracing::error!(
                            "Compression job {} failed. Status message: {}",
                            job_id,
                            status_message.as_deref().unwrap_or("None")
                        );
                        return;
                    }
                    CompressionJobStatus::Killed => {
                        tracing::error!(
                            "Compression job {} was killed. Status message: {}",
                            job_id,
                            status_message.as_deref().unwrap_or("None")
                        );
                        return;
                    }
                    CompressionJobStatus::Succeeded => {
                        break;
                    }
                    _ => {}
                }
            }
            Err(e) => {
                tracing::error!("Failed to fetch compression job status: {}", e);
                return;
            }
        }
    }

    tracing::info!("Compression job {} completed successfully.", job_id);
}
