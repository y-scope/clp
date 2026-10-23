use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

use crate::clp_config::S3Config;
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

#[cfg(test)]
mod tests {
    use serde_json::Value;
    use serde_json::json;

    use super::InputConfig;
    use crate::dataset::DatasetName;

    #[test]
    fn s3_object_metadata_input_config_dataset_is_validated_and_defaults_when_missing_or_null() {
        let base_input_config = json!({
            "type": "s3_object_metadata",
            "bucket": "bucket",
            "region_code": null,
            "key_prefix": "logs/",
            "endpoint_url": null,
            "aws_authentication": {"type": "default"},
            "ingestion_job_id": 1,
            "s3_object_metadata_ids": [1],
            "timestamp_key": null,
            "unstructured": false
        });
        let deserialize_dataset =
            |dataset: Option<Value>| -> Result<DatasetName, serde_json::Error> {
                let mut input_config = base_input_config.clone();
                if let Some(dataset) = dataset {
                    input_config["dataset"] = dataset;
                }
                match serde_json::from_value::<InputConfig>(input_config)? {
                    InputConfig::S3ObjectMetadataInputConfig { config } => Ok(config.dataset),
                    InputConfig::S3InputConfig { .. } => {
                        panic!("the input config should be an S3 object metadata input config")
                    }
                }
            };

        assert_eq!(
            deserialize_dataset(None).expect("input config without a dataset should deserialize"),
            DatasetName::default()
        );
        assert_eq!(
            deserialize_dataset(Some(Value::Null))
                .expect("input config with a null dataset should deserialize"),
            DatasetName::default()
        );
        assert_eq!(
            deserialize_dataset(Some(json!("my_dataset")))
                .expect("input config with a valid dataset should deserialize")
                .as_str(),
            "my_dataset"
        );
        deserialize_dataset(Some(json!("bad-name")))
            .expect_err("input config with an invalid dataset should fail to deserialize");
    }
}
