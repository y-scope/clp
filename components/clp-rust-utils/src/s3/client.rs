use aws_config::BehaviorVersion;
use aws_sdk_s3::{
    Client,
    config::{Builder, Credentials, Region},
};
use non_empty_string::NonEmptyString;

use crate::aws::AWS_DEFAULT_REGION;

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
    region_id: Option<&NonEmptyString>,
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
        .force_path_style(true);
    if endpoint.is_none() && region_id.is_none() {
        // Use the default region from the base config if neither endpoint nor region is provided.
        return Client::from_conf(config_builder.build());
    }
    config_builder.set_region(Some(Region::new(region_id.map_or_else(
        || AWS_DEFAULT_REGION.to_owned(),
        std::string::ToString::to_string,
    ))));
    config_builder.set_endpoint_url(endpoint.map(std::string::ToString::to_string));
    let config = config_builder.build();
    Client::from_conf(config)
}
