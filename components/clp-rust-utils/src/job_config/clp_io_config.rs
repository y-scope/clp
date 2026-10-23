use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

use crate::clp_config::S3Config;
use crate::clp_config::package::config::ArchiveOutput;
use crate::dataset::DatasetName;
use crate::job_config::ingestion::JobId as IngestionJobId;
use crate::s3::S3ObjectMetadataId;

/// Represents CLP IO config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ClpIoConfig {
    pub input: InputConfig,
    pub output: OutputConfig,
}

/// An enum representing CLP input config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "type")]
pub enum InputConfig {
    #[serde(rename = "fs")]
    FsInputConfig {
        #[serde(flatten)]
        config: FsInputConfig,
    },

    #[serde(rename = "s3")]
    S3InputConfig {
        #[serde(flatten)]
        config: S3InputConfig,
    },

    #[serde(rename = "s3_object_metadata")]
    S3ObjectMetadataInputConfig {
        #[serde(flatten)]
        config: S3ObjectMetadataInputConfig,
    },
}

/// Represents filesystem input config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct FsInputConfig {
    pub dataset: DatasetName,
    pub paths_to_compress: Vec<NonEmptyString>,
    // The consumer rejects an explicit `null` for this field, so omit it when unset.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub path_prefix_to_remove: Option<NonEmptyString>,
    pub timestamp_key: Option<NonEmptyString>,
    pub unstructured: bool,
}

/// Represents S3 input config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct S3InputConfig {
    #[serde(flatten)]
    pub s3_config: S3Config,

    pub keys: Option<Vec<NonEmptyString>>,
    pub dataset: DatasetName,
    pub timestamp_key: Option<NonEmptyString>,
    pub unstructured: bool,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct S3ObjectMetadataInputConfig {
    #[serde(flatten)]
    pub s3_config: S3Config,

    pub ingestion_job_id: IngestionJobId,
    pub s3_object_metadata_ids: Vec<S3ObjectMetadataId>,
    pub dataset: DatasetName,
    pub timestamp_key: Option<NonEmptyString>,
    pub unstructured: bool,
}

/// Represents CLP output config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct OutputConfig {
    pub target_archive_size: u64,
    pub target_dictionaries_size: u64,
    pub target_encoded_file_size: u64,
    pub target_segment_size: u64,
    pub compression_level: u8,
}

impl From<&ArchiveOutput> for OutputConfig {
    fn from(archive_output: &ArchiveOutput) -> Self {
        Self {
            target_archive_size: archive_output.target_archive_size,
            target_dictionaries_size: archive_output.target_dictionaries_size,
            target_encoded_file_size: archive_output.target_encoded_file_size,
            target_segment_size: archive_output.target_segment_size,
            compression_level: archive_output.compression_level,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::OutputConfig;
    use crate::clp_config::package::config::ArchiveOutput;

    #[test]
    fn output_config_from_archive_output_maps_every_field() {
        let archive_output = ArchiveOutput {
            target_archive_size: 1,
            target_dictionaries_size: 2,
            target_encoded_file_size: 3,
            target_segment_size: 4,
            compression_level: 5,
            ..ArchiveOutput::default()
        };
        assert_eq!(
            OutputConfig::from(&archive_output),
            OutputConfig {
                target_archive_size: archive_output.target_archive_size,
                target_dictionaries_size: archive_output.target_dictionaries_size,
                target_encoded_file_size: archive_output.target_encoded_file_size,
                target_segment_size: archive_output.target_segment_size,
                compression_level: archive_output.compression_level,
            }
        );
    }
}
