use std::error::Error;

use clp_rust_utils::clp_config::AwsAuthentication;
use clp_rust_utils::clp_config::AwsCredentials;
use clp_rust_utils::clp_config::S3Config;
use clp_rust_utils::job_config::ClpIoConfig;
use clp_rust_utils::job_config::InputConfig;
use clp_rust_utils::job_config::OutputConfig;
use clp_rust_utils::job_config::S3ObjectMetadataInputConfig;
use clp_rust_utils::serde::ZstdMsgpack;
use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
use non_empty_string::NonEmptyString;
use serde_json::Value;

/// Validates CLP I/O config serialization.
///
/// # Errors
///
/// Returns an error if Zstandard compression or `MessagePack` conversion fails.
#[test]
fn test_clp_io_config_serialization() -> Result<(), Box<dyn Error>> {
    let s3_config = S3Config {
        bucket: NonEmptyString::from_static_str("yscope"),
        region_code: Some(NonEmptyString::from_static_str("us-east-2")),
        key_prefix: NonEmptyString::from_static_str("sample-logs/cockroachdb.clp.zst"),
        endpoint_url: None,
        aws_authentication: AwsAuthentication::Credentials {
            credentials: AwsCredentials {
                access_key_id: "ACCESS_KEY_ID".into(),
                secret_access_key: "SECRET_ACCESS_KEY".into(),
                session_token: None,
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

    let zstd_compressed_msgpack = ZstdMsgpack::serialize(&config)?;
    let deserialized_config: Value = ZstdMsgpack::deserialize(&zstd_compressed_msgpack)?;
    assert_eq!(expected, deserialized_config);

    Ok(())
}
