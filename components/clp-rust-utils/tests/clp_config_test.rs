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
            compression_level: 3,
            target_archive_size: 268_435_456,
            target_dictionaries_size: 33_554_432,
            target_encoded_file_size: 268_435_456,
            target_segment_size: 268_435_456,
        },
    };

    let brotli_compressed_msgpack = BrotliMsgpack::serialize(&config)
        .expect("Brotli-compressed MessagePack serialized config.");
    let expected = "1ba80100e4ffdf9f43284b650e496850ba5f98ef7b53044b04d074faa66eb23ebef25c0d1a13ac\
        4b17669ac5cbe254cffc4e749bb4c455d2f61c988e679697db09ba171ce575912c9a14986d0e05f87e4dd1babca\
        9d5b5d1c5267067deea8cacf9928cf673f1a75a627945e06c7cbe6c6b21d98945e16df94ba0d5fce7c2d61158ca\
        6541545e74dd2e9e6e790fd24191a86c1efb597b4f9e82aadfaab0f7f87c81440262811c44fc671677003b39270\
        2be7820095035db7be96769d799a348764444621df0fb4382c2fdba353b5a24ae147e0ba5f3def513120b64bf01\
        aa2df16f31180cafc98d35a4544544c0267abd69708e6ce25962baa581e1cc970a7a2301b070ca0c";
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
        "endpoint_url": null,
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
