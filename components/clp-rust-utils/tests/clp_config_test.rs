use clp_rust_utils::{
    clp_config::{AwsAuthentication, AwsCredentials, S3Config},
    job_config::{ClpIoConfig, InputConfig, OutputConfig, S3InputConfig},
    serde::BrotliMsgpack,
    types::non_empty_string::ExpectedNonEmpty,
};
use non_empty_string::NonEmptyString;
use serde_json::Value;

#[test]
fn test_clp_io_config_serialization() {
    let s3_config = S3Config {
        bucket: NonEmptyString::from_static_str("yscope"),
        region_code: "us-east-2".into(),
        key_prefix: NonEmptyString::from_static_str("sample-logs/cockroachdb.clp.zst"),
        aws_authentication: AwsAuthentication::Credentials {
            credentials: AwsCredentials {
                access_key_id: "ACCESS_KEY_ID".into(),
                secret_access_key: "SECRET_ACCESS_KEY".into(),
            },
        },
    };
    let config = ClpIoConfig {
        input: InputConfig::S3InputConfig {
            config: S3InputConfig {
                s3_config,
                keys: None,
                dataset: Some(NonEmptyString::from_static_str("test-dataset")),
                timestamp_key: Some(NonEmptyString::from_static_str("timestamp")),
                unstructured: false,
            },
        },
        output: OutputConfig {
            tags: None,
            compression_level: 3,
            target_archive_size: 268_435_456,
            target_dictionaries_size: 33_554_432,
            target_encoded_file_size: 268_435_456,
            target_segment_size: 268_435_456,
        },
    };

    let brotli_compressed_msgpack = BrotliMsgpack::serialize(&config)
        .expect("Brotli-compressed MessagePack serialized config.");
    let expected = "1ba00100e4ffdf9f43284b650e496850ba5f1eeefb53844a05d074faa66eb23ebef2dc45638\
                275e9c24cb3bccba29c9bfc9d95db42175d52eecc81793cb3bc3c4ed0bf604c56e5c9a24581d9e65080\
                1fd7263a8fb774fa362adf02eecc5b9d99532b8be8be173f6b659a9538c6c56a15571bc9856e20d0267\
                b1591599975a75cdeb2aea30b83c8b486f3a2b3a74b419d6f99db0742a1482603a9480912e1336f2780\
                dd9c3391503a9205a89a755bfe2c0d3a6be4c98ef0489c0b7e7f2d50b85f8f6e671a54d5dc6fa16d1ac\
                cbaaffc5c3f1fb140f21ba0dce6ff0e8bc5f2da3c58426a9947046ca3cf9a06c7c8219e25a6ad0c4c67\
                b6aceb8c88c782293b";
    assert_eq!(expected, hex::encode(brotli_compressed_msgpack));

    let json_serialized_result = serde_json::to_string_pretty(&config);
    assert!(json_serialized_result.is_ok());
    let json_serialized = json_serialized_result.unwrap();
    let expected = serde_json::json!({
      "input": {
        "type": "s3",
        "bucket": "yscope",
        "region_code": "us-east-2",
        "key_prefix": "sample-logs/cockroachdb.clp.zst",
        "aws_authentication": {
          "type": "credentials",
          "credentials": {
            "access_key_id": "ACCESS_KEY_ID",
            "secret_access_key": "SECRET_ACCESS_KEY"
          }
        },
        "keys": null,
        "dataset": "test-dataset",
        "timestamp_key": "timestamp",
        "unstructured": false
      },
      "output": {
        "tags": null,
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
