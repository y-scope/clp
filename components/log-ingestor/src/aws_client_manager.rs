use anyhow::Result;
use async_trait::async_trait;
use aws_sdk_s3::Client as S3Client;
use aws_sdk_sqs::Client as SqsClient;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use non_empty_string::NonEmptyString;

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
pub trait AwsClientManagerType<Client: AwsClientType>: Send + Sync + Clone + 'static {
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
#[derive(Clone)]
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

    pub async fn create(
        region: Option<&NonEmptyString>,
        access_key_id: &str,
        secret_access_key: &str,
    ) -> Self {
        let sqs_client = clp_rust_utils::sqs::create_new_client(
            access_key_id,
            secret_access_key,
            region.map_or(AWS_DEFAULT_REGION, NonEmptyString::as_str),
            None,
        )
        .await;
        Self::from(sqs_client)
    }
}

/// A simple wrapper around an `S3Client` that implements the `AwsClientManagerType` trait.
#[derive(Clone)]
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

    pub async fn create(
        region: Option<&NonEmptyString>,
        access_key_id: &str,
        secret_access_key: &str,
        endpoint_url: Option<&NonEmptyString>,
    ) -> Self {
        let s3_client = clp_rust_utils::s3::create_new_client(
            access_key_id,
            secret_access_key,
            region.map_or(AWS_DEFAULT_REGION, NonEmptyString::as_str),
            endpoint_url,
        )
        .await;
        Self::from(s3_client)
    }
}
