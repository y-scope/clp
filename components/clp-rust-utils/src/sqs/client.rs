use aws_config::BehaviorVersion;
use aws_sdk_sqs::Client;
use aws_sdk_sqs::config::Builder;
use aws_sdk_sqs::config::Credentials;
use aws_sdk_sqs::config::Region;
use non_empty_string::NonEmptyString;

use crate::clp_config::AwsAuthentication;

/// Creates a new SQS client.
///
/// Credentials come from `aws_authentication`:
///
/// * [`AwsAuthentication::Credentials`] — the given access key pair.
/// * [`AwsAuthentication::Profile`] — the named profile from the shared AWS config files.
/// * [`AwsAuthentication::Default`] and [`AwsAuthentication::EnvVars`] — the default AWS SDK
///   credential provider chain, which consults the standard environment variables first.
///
/// # Notes
///
/// * The client is configured using the latest AWS SDK behavior version.
///
/// # Returns
///
/// A newly created SQS client.
#[must_use]
pub async fn create_new_client(
    region_id: &str,
    endpoint: Option<&NonEmptyString>,
    aws_authentication: &AwsAuthentication,
) -> Client {
    let mut config_defaults =
        aws_config::defaults(BehaviorVersion::latest()).region(Region::new(region_id.to_string()));
    match aws_authentication {
        AwsAuthentication::Credentials { credentials } => {
            config_defaults = config_defaults.credentials_provider(Credentials::new(
                credentials.access_key_id.as_str(),
                credentials.secret_access_key.as_str(),
                credentials.session_token.clone(),
                None,
                "clp-credentials-provider",
            ));
        }
        AwsAuthentication::Profile { profile } => {
            config_defaults = config_defaults.profile_name(profile.as_str());
        }
        AwsAuthentication::Default | AwsAuthentication::EnvVars => {}
    }
    let base_config = config_defaults.load().await;
    let mut config_builder = Builder::from(&base_config);
    config_builder.set_endpoint_url(endpoint.map(std::string::ToString::to_string));
    Client::from_conf(config_builder.build())
}
