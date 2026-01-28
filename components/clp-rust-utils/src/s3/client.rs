use aws_config::BehaviorVersion;
use aws_sdk_s3::{
    Client,
    config::{Builder, Credentials, Region},
};
use non_empty_string::NonEmptyString;

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
    access_key_id: &str,
    secret_access_key: &str,
    region_id: &str,
    endpoint: Option<&NonEmptyString>,
) -> Client {
    let credential = Credentials::new(
        access_key_id,
        secret_access_key,
        None,
        None,
        "clp-credential-provider",
    );
    let base_config = aws_config::defaults(BehaviorVersion::latest())
        .credentials_provider(credential.clone())
        .region(Region::new(region_id.to_string()))
        .load()
        .await;
    let mut config_builder = Builder::from(&base_config).force_path_style(true);
    config_builder.set_endpoint_url(endpoint.map(std::string::ToString::to_string));
    let config = config_builder.build();
    Client::from_conf(config)
}
