use std::future::Future;

use anyhow::Result;
use aws_sdk_s3::Client as S3Client;
use aws_sdk_sqs::Client as SqsClient;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::AwsAuthentication;
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
    fn get(&self) -> impl Future<Output = Result<Client>> + Send;
}

/// A simple wrapper around an `SqsClient` that implements the `AwsClientManagerType` trait.
#[derive(Clone)]
pub struct SqsClientWrapper {
    client: SqsClient,
}

impl AwsClientManagerType<SqsClient> for SqsClientWrapper {
    fn get(&self) -> impl Future<Output = Result<SqsClient>> + Send {
        std::future::ready(Ok(self.client.clone()))
    }
}

impl SqsClientWrapper {
    #[must_use]
    pub const fn from(client: SqsClient) -> Self {
        Self { client }
    }

    pub async fn create(region: Option<&NonEmptyString>, aws_auth: &AwsAuthentication) -> Self {
        let region_str = region.map_or(AWS_DEFAULT_REGION, NonEmptyString::as_str);
        let sqs_client = clp_rust_utils::sqs::create_new_client(region_str, None, aws_auth).await;
        Self::from(sqs_client)
    }
}

/// A simple wrapper around an `S3Client` that implements the `AwsClientManagerType` trait.
#[derive(Clone)]
pub struct S3ClientWrapper {
    client: S3Client,
}

impl AwsClientManagerType<S3Client> for S3ClientWrapper {
    fn get(&self) -> impl Future<Output = Result<S3Client>> + Send {
        std::future::ready(Ok(self.client.clone()))
    }
}

impl S3ClientWrapper {
    #[must_use]
    pub const fn from(client: S3Client) -> Self {
        Self { client }
    }

    pub async fn create(
        region: Option<&NonEmptyString>,
        endpoint_url: Option<&NonEmptyString>,
        aws_auth: &AwsAuthentication,
    ) -> Self {
        let region_str = region.map_or(AWS_DEFAULT_REGION, NonEmptyString::as_str);
        let s3_client =
            clp_rust_utils::s3::create_new_client(region_str, endpoint_url, aws_auth).await;
        Self::from(s3_client)
    }
}
