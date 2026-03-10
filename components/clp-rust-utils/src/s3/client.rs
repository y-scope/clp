use aws_config::BehaviorVersion;
use aws_sdk_s3::{
    Client,
    config::{Builder, Credentials, Region},
};
use non_empty_string::NonEmptyString;

use crate::clp_config::AwsAuthentication;

/// Creates a new S3 client.
///
/// When `aws_authentication` is [`AwsAuthentication::Credentials`], the client uses the given
/// access key pair. When [`AwsAuthentication::Default`], the client uses the default AWS SDK
/// credential provider chain.
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
    region_id: &str,
    endpoint: Option<&NonEmptyString>,
    aws_authentication: &AwsAuthentication,
) -> Client {
    let mut config_defaults =
        aws_config::defaults(BehaviorVersion::latest()).region(Region::new(region_id.to_string()));
    if let AwsAuthentication::Credentials { credentials } = aws_authentication {
        config_defaults = config_defaults.credentials_provider(Credentials::new(
            credentials.access_key_id.as_str(),
            credentials.secret_access_key.as_str(),
            None,
            None,
            "clp-credentials-provider",
        ));
    }
    let base_config = config_defaults.load().await;
    let mut config_builder = Builder::from(&base_config).force_path_style(true);
    config_builder.set_endpoint_url(endpoint.map(std::string::ToString::to_string));
    Client::from_conf(config_builder.build())
}
