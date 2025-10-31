use serde::Serialize;

use crate::clp_config::S3Config;

/// Represents CLP IO config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct ClpIoConfig {
    pub input: InputConfig,
    pub output: OutputConfig,
}

/// An enum representing CLP input config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(tag = "type")]
pub enum InputConfig {
    #[serde(rename = "s3")]
    S3InputConfig {
        #[serde(flatten)]
        config: S3InputConfig,
    },
}

/// Represents S3 input config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct S3InputConfig {
    #[serde(flatten)]
    pub s3_config: S3Config,

    pub keys: Option<Vec<String>>,
    pub dataset: Option<String>,
    pub timestamp_key: Option<String>,
    pub unstructured: bool,
}

/// Represents CLP output config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct OutputConfig {
    pub tags: Option<Vec<String>>,
    pub target_archive_size: u64,
    pub target_dictionaries_size: u64,
    pub target_encoded_file_size: u64,
    pub target_segment_size: u64,
    pub compression_level: u8,
}
