use anyhow::Result;
use async_trait::async_trait;
use aws_sdk_s3::Client as S3Client;
use aws_sdk_sqs::Client as SqsClient;

/// A marker trait for AWS client types.
pub trait AwsClientType {}

impl AwsClientType for SqsClient {}

impl AwsClientType for S3Client {}

/// Trait to provide an abstraction for retrieving AWS SDK clients with capabilities to support
/// different management strategies (e.g., singleton, auto-renew).
///
/// # Type Parameters:
///
/// * [`Client`]: The AWS SKD client type. Must implement the [`AwsClientType`].
#[async_trait]
pub trait AwsClientManagerType<Client: AwsClientType + Clone>: Send + Sync {
    /// Retrieves an AWS client instance. The specific behavior depends on the implementation.
    ///
    /// # Returns:
    ///
    /// A [`Client`] instance ready for use on success.
    ///
    /// # Errors:
    ///
    /// Returns an [`anyhow::Error`] on failure.
    async fn get(&self) -> Result<Client>;
}

/// A simple wrapper around an `SqsClient` that implements the `AwsClientManagerType` trait.
pub struct SqsClientWrapper {
    client: SqsClient,
}

#[async_trait]
impl AwsClientManagerType<SqsClient> for SqsClientWrapper {
    async fn get(&self) -> Result<SqsClient> {
        Ok(self.client.clone())
    }
}

impl SqsClientWrapper {
    #[must_use]
    pub const fn from(client: SqsClient) -> Self {
        Self { client }
    }
}
