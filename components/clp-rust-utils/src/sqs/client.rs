use aws_config::BehaviorVersion;
use aws_sdk_sqs::{
    Client,
    config::{Builder, Credentials, Region},
};
use non_empty_string::NonEmptyString;

/// Creates a new SQS client.
/// The client is configured using the latest AWS SDK behavior version.
///
/// # Returns
///
/// A newly created SQS client.
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
        "clp-credentials-provider",
    );
    let base_config = aws_config::defaults(BehaviorVersion::latest())
        .credentials_provider(credential)
        .region(Region::new(region_id.to_string()))
        .load()
        .await;
    let mut config_builder = Builder::from(&base_config);
    config_builder.set_endpoint_url(endpoint.map(std::string::ToString::to_string));
    Client::from_conf(config_builder.build())
}
