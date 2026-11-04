use aws_sdk_s3::config::Credentials;
use aws_sdk_s3::config::SharedCredentialsProvider;
use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

/// Represents the configuration for connecting to an S3 bucket.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct S3Config {
    pub bucket: NonEmptyString,
    pub region_code: Option<NonEmptyString>,
    pub key_prefix: NonEmptyString,
    pub endpoint_url: Option<NonEmptyString>,
    pub aws_authentication: AwsAuthentication,
}

/// An enum representing AWS authentication methods.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "type")]
pub enum AwsAuthentication {
    /// Uses the default AWS SDK credential provider chain.
    #[serde(rename = "default")]
    Default,

    #[serde(rename = "credentials")]
    Credentials { credentials: AwsCredentials },
}

impl AwsAuthentication {
    /// Creates the credentials provider that AWS clients should install for this authentication
    /// method.
    ///
    /// # Returns
    ///
    /// * A static credentials provider for [`AwsAuthentication::Credentials`].
    /// * `None` for [`AwsAuthentication::Default`], indicating that the default AWS SDK credential
    ///   provider chain should be used.
    #[must_use]
    pub fn credentials_provider(&self) -> Option<SharedCredentialsProvider> {
        match self {
            Self::Credentials { credentials } => {
                Some(SharedCredentialsProvider::new(Credentials::new(
                    credentials.access_key_id.as_str(),
                    credentials.secret_access_key.as_str(),
                    credentials.session_token.clone(),
                    None,
                    "clp-credentials-provider",
                )))
            }
            Self::Default => None,
        }
    }
}

/// Represents AWS credentials.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AwsCredentials {
    pub access_key_id: String,
    pub secret_access_key: String,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub session_token: Option<String>,
}

#[cfg(test)]
mod tests {
    use aws_sdk_s3::config::ProvideCredentials;

    use super::AwsAuthentication;
    use super::AwsCredentials;

    #[tokio::test]
    async fn credentials_provider_is_static_for_credentials_and_absent_for_default() {
        const ACCESS_KEY_ID: &str = "the-access-key";
        const SECRET_ACCESS_KEY: &str = "the-secret-key";
        const SESSION_TOKEN: &str = "the-session-token";

        let auth = AwsAuthentication::Credentials {
            credentials: AwsCredentials {
                access_key_id: ACCESS_KEY_ID.to_owned(),
                secret_access_key: SECRET_ACCESS_KEY.to_owned(),
                session_token: Some(SESSION_TOKEN.to_owned()),
            },
        };

        let credentials = auth
            .credentials_provider()
            .expect("static credentials must install a credentials provider")
            .provide_credentials()
            .await
            .expect("failed to resolve static credentials");

        assert_eq!(credentials.access_key_id(), ACCESS_KEY_ID);
        assert_eq!(credentials.secret_access_key(), SECRET_ACCESS_KEY);
        assert_eq!(credentials.session_token(), Some(SESSION_TOKEN));

        let default_provider = AwsAuthentication::Default.credentials_provider();
        assert!(
            default_provider.is_none(),
            "expected no credentials provider for `default`, got {default_provider:?}"
        );
    }
}
