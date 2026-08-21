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
///
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

    /// Uses the standard AWS environment variables (`AWS_ACCESS_KEY_ID`,
    /// `AWS_SECRET_ACCESS_KEY`, and optionally `AWS_SESSION_TOKEN`). The default SDK credential
    /// provider chain consults those first, so this behaves like [`AwsAuthentication::Default`]
    /// and exists so that configs written by `clp_py_utils` deserialize.
    #[serde(rename = "env_vars")]
    EnvVars,
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
    use super::AwsAuthentication;

    #[test]
    fn deserialize_profile_authentication() {
        let auth: AwsAuthentication = serde_json::from_value(serde_json::json!({
            "type": "profile",
            "profile": "my-profile",
            "credentials": null,
        }))
        .expect("failed to deserialize profile authentication");

        match auth {
            AwsAuthentication::Profile { profile } => assert_eq!(profile.as_str(), "my-profile"),
            other => panic!("expected profile authentication, got {other:?}"),
        }
    }

    #[test]
    fn deserialize_env_vars_authentication() {
        let auth: AwsAuthentication = serde_json::from_value(serde_json::json!({
            "type": "env_vars",
            "profile": null,
            "credentials": null,
        }))
        .expect("failed to deserialize env-var authentication");

        assert_eq!(auth, AwsAuthentication::EnvVars);
    }

    #[test]
    fn deserialize_rejects_an_unknown_authentication_type() {
        let result: Result<AwsAuthentication, _> = serde_json::from_value(serde_json::json!({
            "type": "instance_metadata",
        }));

        assert!(result.is_err());
    }

    #[test]
    fn deserialize_profile_authentication_requires_a_profile() {
        let result: Result<AwsAuthentication, _> =
            serde_json::from_value(serde_json::json!({"type": "profile"}));

        assert!(result.is_err());
    }
}
