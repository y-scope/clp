use std::mem::size_of;

use anyhow::{Context, Result};
use clp_rust_utils::{
    clp_config::{AwsAuthentication, AwsCredentials, S3Config},
    job_config::{ClpIoConfig, InputConfig, OutputConfig, S3ObjectMetadataInputConfig},
    serde::ZstdMsgpack,
    types::non_empty_string::ExpectedNonEmpty,
};
use libzstd_rs_sys::ZSTD_MAGICNUMBER;
use non_empty_string::NonEmptyString;
use serde_json::Value;

/// Validates CLP I/O config serialization.
///
/// # Errors
///
/// Returns an error if serialization, zstd frame validation, or JSON conversion fails.
#[test]
fn test_clp_io_config_serialization() -> Result<()> {
    let s3_config = S3Config {
        bucket: NonEmptyString::from_static_str("yscope"),
        region_code: Some(NonEmptyString::from_static_str("us-east-2")),
        key_prefix: NonEmptyString::from_static_str("sample-logs/cockroachdb.clp.zst"),
        endpoint_url: None,
        aws_authentication: AwsAuthentication::Credentials {
            credentials: AwsCredentials {
                access_key_id: "ACCESS_KEY_ID".into(),
                secret_access_key: "SECRET_ACCESS_KEY".into(),
            },
        },
    };
    let config = ClpIoConfig {
        input: InputConfig::S3ObjectMetadataInputConfig {
            config: S3ObjectMetadataInputConfig {
                s3_config,
                ingestion_job_id: 1,
                s3_object_metadata_ids: vec![],
                dataset: Some(NonEmptyString::from_static_str("test-dataset")),
                timestamp_key: Some(NonEmptyString::from_static_str("timestamp")),
                unstructured: false,
            },
        },
        output: OutputConfig {
            compression_level: 3,
            target_archive_size: 268_435_456,
            target_dictionaries_size: 33_554_432,
            target_encoded_file_size: 268_435_456,
            target_segment_size: 268_435_456,
        },
    };

    let zstd_compressed_msgpack = ZstdMsgpack::serialize(&config)?;
    let frame_magic_bytes: [u8; size_of::<u32>()] = zstd_compressed_msgpack
        .get(..size_of::<u32>())
        .context("zstd frame should contain a magic number")?
        .try_into()
        .context("zstd frame magic number should fit `u32`")?;
    let frame_magic = u32::from_le_bytes(frame_magic_bytes);
    assert_eq!(ZSTD_MAGICNUMBER, frame_magic);

    let json_serialized = serde_json::to_string_pretty(&config)?;
    let expected = serde_json::json!({
      "input": {
        "type": "s3_object_metadata",
        "bucket": "yscope",
        "region_code": "us-east-2",
        "key_prefix": "sample-logs/cockroachdb.clp.zst",
        "endpoint_url": null,
        "aws_authentication": {
          "type": "credentials",
          "credentials": {
            "access_key_id": "ACCESS_KEY_ID",
            "secret_access_key": "SECRET_ACCESS_KEY"
          }
        },
        "ingestion_job_id": 1,
        "s3_object_metadata_ids": [],
        "dataset": "test-dataset",
        "timestamp_key": "timestamp",
        "unstructured": false
      },
      "output": {
        "target_archive_size": 268_435_456,
        "target_dictionaries_size": 33_554_432,
        "target_encoded_file_size": 268_435_456,
        "target_segment_size": 268_435_456,
        "compression_level": 3
      }
    });
    let actual: Value = serde_json::from_str(json_serialized.as_str())?;
    assert_eq!(expected, actual);

    Ok(())
}
