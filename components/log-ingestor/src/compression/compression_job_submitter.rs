use std::sync::Arc;

use anyhow::Result;
use clp_rust_utils::{
    clp_config::{AwsAuthentication::Credentials, AwsCredentials, S3Config},
    database::mysql::create_mysql_pool,
    ingestion_job::S3IngestionBaseConfig,
    job_config::{ClpIoConfig, CompressionJobStatus, OutputConfig, S3InputConfig},
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
    aws_credentials: AwsCredentials,
    output_config: Arc<OutputConfig>,
    ingestion_job_config: S3IngestionBaseConfig,
}

#[async_trait::async_trait]
impl BufferSubmitter for CompressionJobSubmitter {
    async fn submit(&self, buffer: &[ObjectMetadata]) -> Result<()> {
        let s3_config = S3Config {
            bucket: self.ingestion_job_config.bucket_name.clone(),
            region_code: self.ingestion_job_config.region.clone(),
            key_prefix: self.ingestion_job_config.key_prefix.clone(),
            aws_authentication: Credentials {
                credentials: self.aws_credentials.clone(),
            },
        };
        let s3_input_config = S3InputConfig {
            s3_config,
            keys: Some(buffer.iter().map(|obj| obj.key.clone()).collect()),
            dataset: self.ingestion_job_config.dataset.clone(),
            timestamp_key: self.ingestion_job_config.timestamp_key.clone(),
            unstructured: self.ingestion_job_config.unstructured,
        };
        let io_config = ClpIoConfig {
            input: clp_rust_utils::job_config::InputConfig::S3InputConfig {
                config: s3_input_config,
            },
            output: (*self.output_config).clone(),
        };
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
    pub const fn new(
        db_pool: MySqlPool,
        aws_credentials: AwsCredentials,
        output_config: Arc<OutputConfig>,
        ingestion_job_config: S3IngestionBaseConfig,
    ) -> Self {
        Self {
            db_pool,
            aws_credentials,
            output_config,
            ingestion_job_config,
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
                            "Compression job {} failed. Status message: {:?}",
                            job_id,
                            status_message
                        );
                        return;
                    }
                    CompressionJobStatus::Succeeded => {
                        break;
                    }
                    _ => continue,
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
