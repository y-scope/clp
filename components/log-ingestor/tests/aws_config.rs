/// Default AWS configuration for local testing with `LocalStack`.
const DEFAULT_AWS_ENDPOINT_URL: &str = "http://127.0.0.1:4566";
const DEFAULT_AWS_ACCESS_KEY_ID: &str = "test";
const DEFAULT_AWS_SECRET_ACCESS_KEY: &str = "test";
const DEFAULT_AWS_ACCOUNT_ID: &str = "000000000000";
const DEFAULT_AWS_REGION: &str = "us-east-1";

/// AWS service configuration for tests.
pub struct AwsConfig {
    pub endpoint: String,
    pub access_key_id: String,
    pub secret_access_key: String,
    pub region: String,
    pub account_id: String,
    pub bucket_name: String,
    pub queue_name: String,
}

impl AwsConfig {
    /// Loads AWS configuration from environment variables.
    ///
    /// Configurable environment variables:
    ///
    /// * `CLP_LOG_INGESTOR_S3_BUCKET` (REQUIRED): The S3 bucket name for testing log ingestion.
    /// * `CLP_LOG_INGESTOR_SQS_QUEUE` (REQUIRED): The SQS queue name for testing log ingestion.
    /// * `AWS_ENDPOINT_URL`: The AWS service endpoint. Defaults to [`DEFAULT_AWS_ENDPOINT`] if not
    ///   set.
    /// * `AWS_ACCESS_KEY_ID`: The AWS access key ID. Defaults to [`DEFAULT_AWS_ACCESS_KEY_ID`] if
    ///   not set.
    /// * `AWS_SECRET_ACCESS_KEY`: The AWS secret access key. Defaults to
    ///   [`DEFAULT_AWS_SECRET_ACCESS_KEY`] if not set.
    /// * `AWS_REGION`: The AWS region. Defaults to [`DEFAULT_AWS_REGION`] if not set.
    /// * `AWS_ACCOUNT_ID`: The AWS account ID. Defaults to [`DEFAULT_AWS_ACCOUNT_ID`] if not set.
    ///
    /// # Returns
    ///
    /// * `Some(AwsConfig)` if all required environment variables are set.
    /// * `None` if any required environment variable is missing.
    #[must_use]
    pub fn from_env() -> Option<Self> {
        let endpoint = std::env::var("AWS_ENDPOINT_URL")
            .unwrap_or_else(|_| DEFAULT_AWS_ENDPOINT_URL.to_string());
        let access_key_id = std::env::var("AWS_ACCESS_KEY_ID")
            .unwrap_or_else(|_| DEFAULT_AWS_ACCESS_KEY_ID.to_string());
        let secret_access_key = std::env::var("AWS_SECRET_ACCESS_KEY")
            .unwrap_or_else(|_| DEFAULT_AWS_SECRET_ACCESS_KEY.to_string());
        let region = std::env::var("AWS_REGION").unwrap_or_else(|_| DEFAULT_AWS_REGION.to_string());
        let account_id =
            std::env::var("AWS_ACCOUNT_ID").unwrap_or_else(|_| DEFAULT_AWS_ACCOUNT_ID.to_string());

        let Ok(bucket_name) = std::env::var("CLP_LOG_INGESTOR_S3_BUCKET") else {
            return None;
        };

        let Ok(queue_name) = std::env::var("CLP_LOG_INGESTOR_SQS_QUEUE") else {
            return None;
        };

        Some(Self {
            endpoint,
            access_key_id,
            secret_access_key,
            region,
            account_id,
            bucket_name,
            queue_name,
        })
    }
}
