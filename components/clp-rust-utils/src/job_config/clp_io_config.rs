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
    use non_empty_string::NonEmptyString;
    use serde_json::Value;
    use serde_json::json;

    use super::FsInputConfig;
    use super::InputConfig;
    use super::OutputConfig;
    use crate::clp_config::package::config::ArchiveOutput;
    use crate::dataset::DatasetName;
    use crate::types::non_empty_string::ExpectedNonEmpty;

    #[test]
    fn fs_input_config_round_trips_through_named_msgpack() {
        let encoded_input_config = json!({
            "type": "fs",
            "dataset": "my_dataset",
            "paths_to_compress": ["/logs/a.log", "/logs/b.log"],
            "path_prefix_to_remove": "/logs",
            "timestamp_key": "timestamp",
            "unstructured": true
        });
        let serialized =
            rmp_serde::to_vec_named(&encoded_input_config).expect("input config should serialize");
        let input_config: InputConfig =
            rmp_serde::from_slice(&serialized).expect("input config should deserialize");
        assert_eq!(
            input_config,
            InputConfig::FsInputConfig {
                config: FsInputConfig {
                    dataset: "my_dataset"
                        .parse()
                        .expect("name should be a valid dataset name"),
                    paths_to_compress: vec![
                        NonEmptyString::from_static_str("/logs/a.log"),
                        NonEmptyString::from_static_str("/logs/b.log"),
                    ],
                    path_prefix_to_remove: Some(NonEmptyString::from_static_str("/logs")),
                    timestamp_key: Some(NonEmptyString::from_static_str("timestamp")),
                    unstructured: true,
                },
            }
        );

        let reserialized =
            rmp_serde::to_vec_named(&input_config).expect("input config should serialize");
        assert_eq!(
            rmp_serde::from_slice::<Value>(&reserialized)
                .expect("serialized input config should decode as a map"),
            encoded_input_config
        );
    }

    #[test]
    fn fs_input_config_omits_path_prefix_to_remove_when_absent() {
        let input_config = InputConfig::FsInputConfig {
            config: FsInputConfig {
                dataset: DatasetName::default(),
                paths_to_compress: vec![NonEmptyString::from_static_str("/logs/app.log")],
                path_prefix_to_remove: None,
                timestamp_key: None,
                unstructured: false,
            },
        };
        let serialized =
            rmp_serde::to_vec_named(&input_config).expect("input config should serialize");
        assert_eq!(
            rmp_serde::from_slice::<Value>(&serialized)
                .expect("serialized input config should decode as a map"),
            json!({
                "type": "fs",
                "dataset": DatasetName::default_name(),
                "paths_to_compress": ["/logs/app.log"],
                "timestamp_key": null,
                "unstructured": false
            })
        );
    }

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
