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
    region_id: Option<&NonEmptyString>,
    access_key_id: &str,
    secret_access_key: &str,
    endpoint: Option<&NonEmptyString>,
) -> Client {
    let credential = Credentials::new(
        access_key_id,
        secret_access_key,
        None,
        None,
        "clp-credential-provider",
    );
    let base_config = aws_config::defaults(BehaviorVersion::latest()).load().await;
    let mut config_builder = Builder::from(&base_config)
        .credentials_provider(credential)
        .region(region_id.map(|region| Region::new(region.to_string())));
    config_builder.set_endpoint_url(endpoint.map(std::string::ToString::to_string));
    Client::from_conf(config_builder.build())
}
