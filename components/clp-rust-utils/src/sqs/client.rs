use aws_config::BehaviorVersion;
use aws_sdk_sqs::{
    Client,
    config::{Builder, Credentials, Region},
};
use secrecy::{ExposeSecret, SecretString};

/// Creates a new SQS client.
/// The client is configured using the latest AWS SDK behavior version.
///
/// # Returns
///
/// A newly created SQS client.
#[must_use]
pub async fn create_new_client(
    region_id: &str,
    access_key_id: &str,
    secret_access_key: &SecretString,
) -> Client {
    let credentials = Credentials::new(
        access_key_id,
        secret_access_key.expose_secret(),
        None,
        None,
        "clp-credential-provider",
    );
    let region = Region::new(region_id.to_owned());
    let base_config = aws_config::defaults(BehaviorVersion::latest())
        .region(region.clone())
        .load()
        .await;
    let config = Builder::from(&base_config)
        .credentials_provider(credentials)
        .region(region)
        .build();
    Client::from_conf(config)
}
