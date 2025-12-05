use aws_config::BehaviorVersion;
use aws_sdk_s3::{
    Client,
    config::{Builder, Credentials, Region},
};

/// Creates a new S3 client.
///
/// # Notes
///
/// * The client is configured using the latest AWS SDK behavior version.
/// * The client enforces path-style addressing.
///
/// # Returns
///
/// A newly created S3 client.
#[must_use]
pub async fn create_new_client(
    endpoint: &str,
    region_id: &str,
    access_key_id: &str,
    secret_access_key: &str,
) -> Client {
    let credential = Credentials::new(
        access_key_id,
        secret_access_key,
        None,
        None,
        "clp-credential-provider",
    );
    let region = Region::new(region_id.to_owned());
    let base_config = aws_config::defaults(BehaviorVersion::latest()).load().await;
    let config = Builder::from(&base_config)
        .endpoint_url(endpoint)
        .region(region)
        .credentials_provider(credential)
        .force_path_style(true)
        .build();
    Client::from_conf(config)
}
