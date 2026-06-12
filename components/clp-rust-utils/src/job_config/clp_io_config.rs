use non_empty_string::NonEmptyString;
use serde::Serialize;

use crate::{
    clp_config::{
        AwsAuthentication,
        S3Config,
        package::{DEFAULT_DATASET_NAME, config::ArchiveOutput},
    },
    job_config::ingestion::{JobId as IngestionJobId, s3::BaseConfig},
    s3::S3ObjectMetadataId,
};

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
    pub s3_object_metadata_ids: Vec<S3ObjectMetadataId>,
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

impl ClpIoConfig {
    /// Creates a CLP IO config for compressing previously ingested S3 object metadata.
    ///
    /// # Returns
    ///
    /// A [`ClpIoConfig`] configured to read from the given S3 object metadata IDs and write using
    /// the given archive output configuration.
    #[must_use]
    pub fn from_ingested_s3_object_metadata(
        aws_authentication: AwsAuthentication,
        archive_output_config: &ArchiveOutput,
        ingestion_job_config: &BaseConfig,
        ingestion_job_id: IngestionJobId,
        s3_object_metadata_ids: Vec<S3ObjectMetadataId>,
    ) -> Self {
        let s3_object_metadata_input_config = S3ObjectMetadataInputConfig {
            s3_config: S3Config {
                bucket: ingestion_job_config.bucket_name.clone(),
                region_code: ingestion_job_config.region.clone(),
                key_prefix: ingestion_job_config.key_prefix.clone(),
                endpoint_url: ingestion_job_config.endpoint_url.clone(),
                aws_authentication,
            },
            ingestion_job_id,
            s3_object_metadata_ids,
            // NOTE: Workaround for #1735
            dataset: Some(
                ingestion_job_config
                    .dataset
                    .clone()
                    .unwrap_or_else(|| DEFAULT_DATASET_NAME.clone()),
            ),
            timestamp_key: ingestion_job_config.timestamp_key.clone(),
            unstructured: ingestion_job_config.unstructured,
        };
        let output_config = OutputConfig {
            target_archive_size: archive_output_config.target_archive_size,
            target_dictionaries_size: archive_output_config.target_dictionaries_size,
            target_encoded_file_size: archive_output_config.target_encoded_file_size,
            target_segment_size: archive_output_config.target_segment_size,
            compression_level: archive_output_config.compression_level,
        };

        Self {
            input: InputConfig::S3ObjectMetadataInputConfig {
                config: s3_object_metadata_input_config,
            },
            output: output_config,
        }
    }
}
