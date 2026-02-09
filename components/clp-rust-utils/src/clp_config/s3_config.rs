use non_empty_string::NonEmptyString;
use serde::{Deserialize, Serialize};

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
    #[serde(rename = "credentials")]
    Credentials { credentials: AwsCredentials },

    /// Uses the default AWS SDK credential provider chain.
    #[serde(rename = "ec2")]
    Ec2,
}

impl AwsAuthentication {
    /// Returns the access key pair as `Some((access_key_id, secret_access_key))` for explicit
    /// credentials, or `None` for authentication methods that rely on the default credential
    /// provider chain.
    pub fn credentials_pair(&self) -> Option<(&str, &str)> {
        match self {
            Self::Credentials { credentials } => Some((
                credentials.access_key_id.as_str(),
                credentials.secret_access_key.as_str(),
            )),
            Self::Ec2 => None,
        }
    }
}

/// Represents AWS credentials.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AwsCredentials {
    pub access_key_id: String,
    pub secret_access_key: String,
}
