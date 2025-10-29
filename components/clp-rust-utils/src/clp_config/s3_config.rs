use serde::Serialize;

use crate::clp_config::AwsAuthentication;

/// Represents the configuration for connecting to an S3 bucket.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct S3Config {
    pub bucket: String,
    pub region_code: String,
    pub key_prefix: String,
    pub aws_authentication: AwsAuthentication,
}
