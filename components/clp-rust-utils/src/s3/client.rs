use aws_config::BehaviorVersion;
use aws_sdk_s3::Client;
use aws_sdk_s3::config::Builder;
use aws_sdk_s3::config::Region;
use non_empty_string::NonEmptyString;

use crate::clp_config::AwsAuthentication;

/// Creates a new S3 client.
///
/// The client authenticates with the credentials provider from
/// [`AwsAuthentication::credentials_provider`], or with the default AWS SDK credential provider
/// chain if there is none.
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
    if let Some(credentials_provider) = aws_authentication.credentials_provider(region_id) {
        config_defaults = config_defaults.credentials_provider(credentials_provider);
    }
    let base_config = config_defaults.load().await;
    let mut config_builder = Builder::from(&base_config).force_path_style(true);
    config_builder.set_endpoint_url(endpoint.map(std::string::ToString::to_string));
    Client::from_conf(config_builder.build())
}

#[cfg(test)]
mod tests {
    use super::create_new_client;
    use crate::aws::AWS_DEFAULT_REGION;
    use crate::aws::test_utils::AuthorizationCapture;
    use crate::clp_config::AwsAuthentication;
    use crate::clp_config::AwsCredentials;

    #[tokio::test]
    async fn client_signs_requests_with_static_credentials() {
        const ACCESS_KEY_ID: &str = "AKIDCLPTEST";

        let auth = AwsAuthentication::Credentials {
            credentials: AwsCredentials {
                access_key_id: ACCESS_KEY_ID.to_owned(),
                secret_access_key: "the-secret-key".to_owned(),
                session_token: None,
            },
        };
        let client = create_new_client(AWS_DEFAULT_REGION, None, &auth).await;
        let capture = AuthorizationCapture::default();

        client
            .list_buckets()
            .customize()
            .interceptor(capture.clone())
            .send()
            .await
            .expect_err("the interceptor must abort the request before transmission");

        let authorization = capture
            .authorization()
            .expect("the request must be signed before transmission");
        assert!(
            authorization.contains(&format!("Credential={ACCESS_KEY_ID}/")),
            "expected the request to be signed with `{ACCESS_KEY_ID}`, got `{authorization}`"
        );
    }
}
