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
}

/// Represents AWS credentials.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AwsCredentials {
    pub access_key_id: String,
    pub secret_access_key: String,
}
