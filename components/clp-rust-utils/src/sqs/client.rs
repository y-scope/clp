use aws_config::BehaviorVersion;
use aws_sdk_sqs::{
    Client,
    config::{Builder, Credentials, Region},
};
use non_empty_string::NonEmptyString;

/// Creates a new SQS client.
///
/// When `credentials` is `Some`, the client uses the given access key pair. When `None`, the client
/// uses the default AWS SDK credential provider chain.
///
/// The client is configured using the latest AWS SDK behavior version.
///
/// # Returns
///
/// A newly created SQS client.
#[must_use]
pub async fn create_new_client(
    region_id: &str,
    endpoint: Option<&NonEmptyString>,
    credentials: Option<(&str, &str)>,
) -> Client {
    let mut config_defaults = aws_config::defaults(BehaviorVersion::latest())
        .region(Region::new(region_id.to_string()));
    if let Some((access_key_id, secret_access_key)) = credentials {
        config_defaults = config_defaults.credentials_provider(Credentials::new(
            access_key_id,
            secret_access_key,
            None,
            None,
            "clp-credentials-provider",
        ));
    }
    let base_config = config_defaults.load().await;
    let mut config_builder = Builder::from(&base_config);
    config_builder.set_endpoint_url(endpoint.map(std::string::ToString::to_string));
    Client::from_conf(config_builder.build())
}
