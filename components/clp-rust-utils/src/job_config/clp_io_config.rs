use non_empty_string::NonEmptyString;
use serde::Serialize;

use crate::{clp_config::S3Config, s3::S3ObjectMetadataId};

/// Represents CLP IO config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct ClpIoConfig {
    pub input: InputConfig,
    pub output: OutputConfig,
}

/// Represents the unique identifier for an ingestion job in CLP DB.
pub type IngestionJobId = u64;

/// An enum representing CLP input config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(tag = "type")]
pub enum InputConfig {
    #[serde(rename = "s3_object_metadata")]
    S3ObjectMetadataInputConfig {
        #[serde(flatten)]
        config: S3ObjectMetadataInputConfig,
    },
}

/// Represents S3 input config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct S3InputConfig {
    #[serde(flatten)]
    pub s3_config: S3Config,

    pub keys: Option<Vec<NonEmptyString>>,
    pub dataset: Option<NonEmptyString>,
    pub timestamp_key: Option<NonEmptyString>,
    pub unstructured: bool,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct S3ObjectMetadataInputConfig {
    #[serde(flatten)]
    pub s3_config: S3Config,

    pub ingestion_job_id: IngestionJobId,
    pub s3_object_metadata_ids: Option<Vec<S3ObjectMetadataId>>,
    pub dataset: Option<NonEmptyString>,
    pub timestamp_key: Option<NonEmptyString>,
    pub unstructured: bool,
}

/// Represents CLP output config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct OutputConfig {
    pub target_archive_size: u64,
    pub target_dictionaries_size: u64,
    pub target_encoded_file_size: u64,
    pub target_segment_size: u64,
    pub compression_level: u8,
}
