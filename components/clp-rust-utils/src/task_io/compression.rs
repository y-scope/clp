//! Protocol types exchanged with the Spider (Huntsman) tasks that run CLP S3 compression jobs.

use non_empty_string::NonEmptyString;
use serde::{Deserialize, Serialize};

use crate::clp_config::AwsAuthentication;

/// `clp-s` tuning and engine options for a compression job.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ClpSCompressionOption {
    pub target_encoded_size: u64,
    pub compression_level: u8,
    pub timestamp_key: Option<String>,
    pub unstructured: bool,
}

/// Input source for a compression task.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct S3InputSource {
    pub endpoint_url: Option<NonEmptyString>,
    pub region_code: Option<NonEmptyString>,
    pub bucket: NonEmptyString,
    pub aws_authentication: AwsAuthentication,
    pub object_keys: Vec<String>,
}

/// Output of a compression task.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct CompressionTaskOutput {
    pub dataset: Option<String>,
    pub archives: Vec<ArchiveMetadata>,
}

/// Metadata of an archive produced by `clp-s`.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ArchiveMetadata {
    pub id: String,
    pub begin_timestamp: i64,
    pub end_timestamp: i64,
    pub size: i64,
    pub uncompressed_size: i64,
}
