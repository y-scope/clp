use anyhow::{Result, anyhow};
use non_empty_string::NonEmptyString;

/// Default AWS configuration for local testing with `LocalStack`.
const DEFAULT_AWS_ENDPOINT_URL: &str = "http://127.0.0.1:4566";
const DEFAULT_AWS_ACCESS_KEY_ID: &str = "test";
const DEFAULT_AWS_SECRET_ACCESS_KEY: &str = "test";
const DEFAULT_AWS_ACCOUNT_ID: &str = "000000000000";
const DEFAULT_AWS_REGION: &str = "us-east-1";

/// AWS service configuration for tests.
#[derive(Clone)]
pub struct AwsConfig {
    pub endpoint: NonEmptyString,
    pub access_key_id: String,
    pub secret_access_key: String,
    pub region: NonEmptyString,
    pub account_id: String,
    pub bucket_name: NonEmptyString,
    pub queue_name: NonEmptyString,
}

impl AwsConfig {
    /// Loads AWS configuration from environment variables.
    ///
    /// Configurable environment variables:
    ///
    /// * `CLP_LOG_INGESTOR_S3_BUCKET` (REQUIRED): The S3 bucket name for testing log ingestion.
    /// * `CLP_LOG_INGESTOR_SQS_QUEUE` (REQUIRED): The SQS queue name for testing log ingestion.
    /// * `AWS_ENDPOINT_URL`: The AWS service endpoint. Defaults to [`DEFAULT_AWS_ENDPOINT_URL`] if
    ///   not set.
    /// * `AWS_ACCESS_KEY_ID`: The AWS access key ID. Defaults to [`DEFAULT_AWS_ACCESS_KEY_ID`] if
    ///   not set.
    /// * `AWS_SECRET_ACCESS_KEY`: The AWS secret access key. Defaults to
    ///   [`DEFAULT_AWS_SECRET_ACCESS_KEY`] if not set.
    /// * `AWS_REGION`: The AWS region. Defaults to [`DEFAULT_AWS_REGION`] if not set.
    /// * `AWS_ACCOUNT_ID`: The AWS account ID. Defaults to [`DEFAULT_AWS_ACCOUNT_ID`] if not set.
    ///
    /// # Returns
    ///
    /// An [`AwsConfig`] containing the required environment variables on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * `CLP_LOG_INGESTOR_S3_BUCKET` environment variable is not set, or set to an empty string.
    /// * `CLP_LOG_INGESTOR_SQS_QUEUE` environment variable is not set, or set to an empty string.
    pub fn from_env() -> Result<Self> {
        let endpoint = std::env::var("AWS_ENDPOINT_URL")
            .unwrap_or_else(|_| DEFAULT_AWS_ENDPOINT_URL.to_string());
        let access_key_id = std::env::var("AWS_ACCESS_KEY_ID")
            .unwrap_or_else(|_| DEFAULT_AWS_ACCESS_KEY_ID.to_string());
        let secret_access_key = std::env::var("AWS_SECRET_ACCESS_KEY")
            .unwrap_or_else(|_| DEFAULT_AWS_SECRET_ACCESS_KEY.to_string());
        let region = std::env::var("AWS_REGION").unwrap_or_else(|_| DEFAULT_AWS_REGION.to_string());
        let account_id =
            std::env::var("AWS_ACCOUNT_ID").unwrap_or_else(|_| DEFAULT_AWS_ACCOUNT_ID.to_string());

        let bucket_name = std::env::var("CLP_LOG_INGESTOR_S3_BUCKET").map_err(|_| {
            anyhow!("`CLP_LOG_INGESTOR_S3_BUCKET` environment variable is not set.")
        })?;
        let queue_name = std::env::var("CLP_LOG_INGESTOR_SQS_QUEUE").map_err(|_| {
            anyhow!("`CLP_LOG_INGESTOR_SQS_QUEUE` environment variable is not set.")
        })?;

        Ok(Self {
            endpoint: NonEmptyString::new(endpoint)
                .map_err(|_| anyhow!("endpoint must not be empty"))?,
            access_key_id,
            secret_access_key,
            region: NonEmptyString::new(region).map_err(|_| anyhow!("region must not be empty"))?,
            account_id,
            bucket_name: NonEmptyString::new(bucket_name)
                .map_err(|_| anyhow!("bucket name must not be empty"))?,
            queue_name: NonEmptyString::new(queue_name)
                .map_err(|_| anyhow!("queue name must not be empty"))?,
        })
    }
}
