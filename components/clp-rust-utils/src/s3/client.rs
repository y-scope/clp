use aws_config::BehaviorVersion;
use aws_sdk_s3::{
    Client,
    config::{Builder, Credentials, Region},
};
use secrecy::{ExposeSecret, SecretString};

/// Creates a new S3 client.
///
/// # Returns
/// A newly created S3 client.
#[must_use]
pub async fn create_new_client(
    endpoint: &str,
    region_id: &str,
    access_key_id: &str,
    secret_access_key: &SecretString,
) -> Client {
    let credential = Credentials::new(
        access_key_id,
        secret_access_key.expose_secret(),
        None,
        None,
        "clp-user",
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
