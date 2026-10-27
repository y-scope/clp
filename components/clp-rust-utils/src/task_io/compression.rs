//! Protocol types exchanged with the Spider (Huntsman) tasks that run CLP compression jobs.

use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

use crate::clp_config::AwsAuthentication;
use crate::dataset::DatasetName;
use crate::types::ArchiveId;

/// `clp-s` tuning and engine options for a compression job.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ClpSCompressionOption {
    pub target_encoded_size: u64,
    pub compression_level: u8,
    pub timestamp_key: Option<String>,
    pub unstructured: bool,
}

/// Input manifest for a compression task.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "type", content = "source", rename_all = "snake_case")]
pub enum CompressionInputSource {
    Fs(FsInputSource),
    S3(S3InputSource),
}

impl CompressionInputSource {
    /// # Returns
    ///
    /// The file paths or object keys assigned to the task.
    #[must_use]
    pub fn paths(&self) -> &[String] {
        match self {
            Self::Fs(source) => &source.file_paths,
            Self::S3(source) => &source.object_keys,
        }
    }
}

/// Filesystem entries assigned to one compression task.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct FsInputSource {
    pub file_paths: Vec<String>,
    pub empty_directories: Vec<String>,
}

/// S3 objects assigned to one compression task.
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
    pub dataset: DatasetName,
    pub archives: Vec<ArchiveMetadata>,
}

/// Metadata of an archive produced by `clp-s`.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ArchiveMetadata {
    pub id: ArchiveId,
    pub begin_timestamp: i64,
    pub end_timestamp: i64,
    pub size: i64,
    pub uncompressed_size: i64,
}
