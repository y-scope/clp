use clp_rust_utils::{
    clp_config::{AwsAuthentication, AwsCredentials, S3Config},
    job_config::{ClpIoConfig, InputConfig, OutputConfig, S3ObjectMetadataInputConfig},
    serde::BrotliMsgpack,
    types::non_empty_string::ExpectedNonEmpty,
};
use non_empty_string::NonEmptyString;
use serde_json::Value;

#[test]
fn test_clp_io_config_serialization() {
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

    let brotli_compressed_msgpack = BrotliMsgpack::serialize(&config)
        .expect("Brotli-compressed MessagePack serialized config.");

    let expected = "1bdc0100c4aa350b081365c242113820d5873cbb21498afe8d607454fea76eb23ebef2d88d496b8\
        0fc651b33cde2657128e936efe894546b2a5dab7075ae18eece282fdd0b15f15380066703935c861712976d955d\
        2c20b16e7d2ad8d7d79cd2dbeda70b37b6ba0096d677b68cb4e3eb94ed932f912fca8622656427c82c89dda99ef\
        2d6fc2978e4fcbc883382c8b14d982917815e6d26bc9539493b1e334587e468750fa642d47753b92f552ab55e0f\
        9dda04adea33aa1ba81d3cf76a956ead0fd5fcc9f45ef2ffc3f760f9124c25fe716e03a642a797d2945476c7546\
        2e9e1de37160ae15ecfe28e7b60a7cadf4575c4dbd78f4b6c24e809642ed5ff542c16fb5a5cd1495253250d3841\
        af440db6c9ca9e39a66d0034cfb2efd6083a204eb64a02";

    assert_eq!(expected, hex::encode(brotli_compressed_msgpack));

    let json_serialized_result = serde_json::to_string_pretty(&config);
    assert!(json_serialized_result.is_ok());
    let json_serialized = json_serialized_result.unwrap();
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
    let actual: Value = serde_json::from_str(json_serialized.as_str())
        .expect("The serialization result should be a valid JSON string.");
    assert_eq!(expected, actual);
}
