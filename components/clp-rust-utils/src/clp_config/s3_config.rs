use aws_config::Region;
use aws_config::profile::ProfileFileCredentialsProvider;
use aws_config::provider_config::ProviderConfig;
use aws_runtime::env_config::file::EnvConfigFiles;
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

/// Mirror of `clp_py_utils.clp_config.AwsAuthType`. Must be kept in sync.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "type")]
pub enum AwsAuthentication {
    /// Uses the default AWS SDK credential provider chain.
    #[serde(rename = "default")]
    Default,

    #[serde(rename = "credentials")]
    Credentials { credentials: AwsCredentials },

    /// Uses the named profile from the shared AWS config and credentials files.
    #[serde(rename = "profile")]
    Profile { profile: NonEmptyString },

    /// Uses the standard AWS credential environment variables, resolved through the default AWS
    /// SDK credential provider chain.
    #[serde(rename = "env_vars")]
    EnvVars,
}

impl AwsAuthentication {
    /// Creates the credentials provider that AWS clients should install for this authentication
    /// method.
    ///
    /// # Returns
    ///
    /// * The credentials provider for [`AwsAuthentication::Credentials`] and
    ///   [`AwsAuthentication::Profile`].
    /// * `None` for [`AwsAuthentication::Default`] and [`AwsAuthentication::EnvVars`], indicating
    ///   that the default AWS SDK credential provider chain should be used.
    #[must_use]
    pub fn credentials_provider(&self, region: &str) -> Option<SharedCredentialsProvider> {
        self.credentials_provider_with_provider_config(
            region,
            ProviderConfig::default(),
            EnvConfigFiles::default(),
        )
    }

    /// Creates the credentials provider that AWS clients should install for this authentication
    /// method, resolving named profiles from `profile_files` through `provider_config`.
    ///
    /// # Returns
    ///
    /// See [`AwsAuthentication::credentials_provider`].
    fn credentials_provider_with_provider_config(
        &self,
        region: &str,
        provider_config: ProviderConfig,
        profile_files: EnvConfigFiles,
    ) -> Option<SharedCredentialsProvider> {
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
            Self::Profile { profile } => Some(SharedCredentialsProvider::new(
                ProfileFileCredentialsProvider::builder()
                    .configure(&provider_config.with_region(Some(Region::new(region.to_owned()))))
                    .profile_name(profile.as_str())
                    .profile_files(profile_files)
                    .build(),
            )),
            Self::Default | Self::EnvVars => None,
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
    use std::path::Path;

    use aws_config::provider_config::ProviderConfig;
    use aws_runtime::env_config::file::EnvConfigFileKind;
    use aws_runtime::env_config::file::EnvConfigFiles;
    use aws_sdk_s3::config::ProvideCredentials;
    use non_empty_string::NonEmptyString;

    use super::AwsAuthentication;
    use super::AwsCredentials;
    use crate::aws::AWS_DEFAULT_REGION;

    /// Writes the given credentials and config files into `dir`.
    ///
    /// # Returns
    ///
    /// The profile files that resolve profiles from the written files.
    ///
    /// # Panics
    ///
    /// Panics if either file can't be written.
    fn write_profile_files(dir: &Path, credentials: &[&str], config: &[&str]) -> EnvConfigFiles {
        let credentials_file = dir.join("credentials");
        let config_file = dir.join("config");
        std::fs::write(&credentials_file, credentials.join("\n"))
            .expect("failed to write credentials file");
        std::fs::write(&config_file, config.join("\n")).expect("failed to write config file");
        EnvConfigFiles::builder()
            .with_file(EnvConfigFileKind::Credentials, &credentials_file)
            .with_file(EnvConfigFileKind::Config, &config_file)
            .build()
    }

    #[test]
    fn profile_authentication_deserializes_with_null_credentials() {
        let auth: AwsAuthentication = serde_json::from_value(serde_json::json!({
            "type": "profile",
            "profile": "my-profile",
            "credentials": null,
        }))
        .expect("failed to deserialize profile authentication");

        assert_eq!(
            auth,
            AwsAuthentication::Profile {
                profile: NonEmptyString::new("my-profile".to_owned())
                    .expect("profile name must be non-empty"),
            }
        );
    }

    #[test]
    fn env_vars_authentication_deserializes_with_null_profile_and_credentials() {
        let auth: AwsAuthentication = serde_json::from_value(serde_json::json!({
            "type": "env_vars",
            "profile": null,
            "credentials": null,
        }))
        .expect("failed to deserialize env-vars authentication");

        assert_eq!(auth, AwsAuthentication::EnvVars);
    }

    #[test]
    fn unknown_authentication_type_is_rejected() {
        serde_json::from_value::<AwsAuthentication>(serde_json::json!({
            "type": "instance_metadata",
        }))
        .expect_err("an unknown authentication type must be rejected");
    }

    #[test]
    fn profile_authentication_without_profile_is_rejected() {
        serde_json::from_value::<AwsAuthentication>(serde_json::json!({"type": "profile"}))
            .expect_err("profile authentication without a profile must be rejected");
    }

    #[tokio::test]
    async fn profile_authentication_resolves_named_profile_keys() {
        const PROFILE: &str = "explicit";
        const ACCESS_KEY_ID: &str = "profile-access";
        const SECRET_ACCESS_KEY: &str = "profile-secret";

        let profile_dir = tempfile::tempdir().expect("failed to create profile directory");
        let profile_files = write_profile_files(
            profile_dir.path(),
            &[
                "[default]",
                "aws_access_key_id = default-access",
                "aws_secret_access_key = default-secret",
                &format!("[{PROFILE}]"),
                &format!("aws_access_key_id = {ACCESS_KEY_ID}"),
                &format!("aws_secret_access_key = {SECRET_ACCESS_KEY}"),
            ],
            &[],
        );
        let auth = AwsAuthentication::Profile {
            profile: NonEmptyString::new(PROFILE.to_owned())
                .expect("profile name must be non-empty"),
        };

        let credentials = auth
            .credentials_provider_with_provider_config(
                AWS_DEFAULT_REGION,
                ProviderConfig::default(),
                profile_files,
            )
            .expect("profile authentication must install a credentials provider")
            .provide_credentials()
            .await
            .expect("failed to resolve named profile credentials");

        assert_eq!(credentials.access_key_id(), ACCESS_KEY_ID);
        assert_eq!(credentials.secret_access_key(), SECRET_ACCESS_KEY);
        assert_eq!(credentials.session_token(), None);
    }

    #[tokio::test]
    async fn credentials_provider_resolves_static_credentials() {
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
            .credentials_provider(AWS_DEFAULT_REGION)
            .expect("static credentials must install a credentials provider")
            .provide_credentials()
            .await
            .expect("failed to resolve static credentials");

        assert_eq!(credentials.access_key_id(), ACCESS_KEY_ID);
        assert_eq!(credentials.secret_access_key(), SECRET_ACCESS_KEY);
        assert_eq!(credentials.session_token(), Some(SESSION_TOKEN));
    }

    #[test]
    fn credentials_provider_is_absent_only_for_default_chain_authentication() {
        let profile = AwsAuthentication::Profile {
            profile: NonEmptyString::new("test-profile".to_owned())
                .expect("profile name must be non-empty"),
        };
        let credentials = AwsAuthentication::Credentials {
            credentials: AwsCredentials {
                access_key_id: "the-access-key".to_owned(),
                secret_access_key: "the-secret-key".to_owned(),
                session_token: None,
            },
        };

        assert!(profile.credentials_provider(AWS_DEFAULT_REGION).is_some());
        assert!(
            credentials
                .credentials_provider(AWS_DEFAULT_REGION)
                .is_some()
        );
        for auth in [AwsAuthentication::Default, AwsAuthentication::EnvVars] {
            let provider = auth.credentials_provider(AWS_DEFAULT_REGION);
            assert!(
                provider.is_none(),
                "expected no credentials provider for {auth:?}, got {provider:?}"
            );
        }
    }
}
