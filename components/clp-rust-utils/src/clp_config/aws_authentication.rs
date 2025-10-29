use serde::Serialize;

/// An enum representing AWS authentication methods.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(tag = "type")]
pub enum AwsAuthentication {
    #[serde(rename = "credentials")]
    Credentials { credentials: Option<AwsCredentials> },
}

/// Represents AWS credentials.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct AwsCredentials {
    pub access_key_id: String,
    pub secret_access_key: String,
}
