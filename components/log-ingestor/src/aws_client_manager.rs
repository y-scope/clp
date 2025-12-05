use anyhow::Result;
use async_trait::async_trait;
use aws_sdk_s3::Client as S3Client;
use aws_sdk_sqs::Client as SqsClient;
use clp_rust_utils::{
    s3::create_new_client as create_s3_client,
    sqs::create_new_client as create_sqs_client,
};

/// A marker trait for AWS client types.
pub trait AwsClientType: Clone {}

impl AwsClientType for SqsClient {}

impl AwsClientType for S3Client {}

/// Trait to provide an abstraction for retrieving AWS SDK clients with capabilities to support
/// different management strategies (e.g., singleton, auto-renew).
///
/// # Type Parameters:
///
/// * [`Client`]: The AWS SKD client type. Must implement the [`AwsClientType`].
#[async_trait]
pub trait AwsClientManagerType<Client: AwsClientType>: Send + Sync + 'static {
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

    pub async fn create(region: &str, access_key_id: &str, secret_access_key: &str) -> Self {
        let sqs_endpoint = format!("https://sqs.{region}.amazonaws.com");
        let sqs_client = create_sqs_client(
            sqs_endpoint.as_str(),
            region,
            access_key_id,
            secret_access_key,
        )
        .await;
        Self::from(sqs_client)
    }
}

/// A simple wrapper around an `S3Client` that implements the `AwsClientManagerType` trait.
pub struct S3ClientWrapper {
    client: S3Client,
}

#[async_trait]
impl AwsClientManagerType<S3Client> for S3ClientWrapper {
    async fn get(&self) -> Result<S3Client> {
        Ok(self.client.clone())
    }
}

impl S3ClientWrapper {
    #[must_use]
    pub const fn from(client: S3Client) -> Self {
        Self { client }
    }

    pub async fn create(region: &str, access_key_id: &str, secret_access_key: &str) -> Self {
        let s3_endpoint = format!("https://s3.{region}.amazonaws.com");
        let s3_client = create_s3_client(
            s3_endpoint.as_str(),
            region,
            access_key_id,
            secret_access_key,
        )
        .await;
        Self::from(s3_client)
    }
}
