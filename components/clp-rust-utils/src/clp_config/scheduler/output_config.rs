use serde::Serialize;

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
