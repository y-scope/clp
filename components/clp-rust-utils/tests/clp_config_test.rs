use clp_rust_utils::{
    clp_config::{AwsAuthentication, AwsCredentials, S3Config},
    job_config::{ClpIoConfig, InputConfig, S3ObjectMetadataInputConfig, OutputConfig},
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
                metadata_ids: None,
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
    let expected = "1bc80100e4ffbf9f0b909134588545d1fdd9f7bd15164db4d3eaf8a66eb\
        23ebef29a6d1a13ec2fdb9869162f8bd36dded129a9e934ba56c970ae984c6794977ea55a32d3e6c6\
        5b7094d745b2e80361b69e1458756b8ae12087e1c44617fb80a079d7c80b1bb68cd1d3f1c7b3595e1\
        1b8086f389101929d68092fcc5f02f58ccf85ed22b014d19ca87ce9f5308464cb33910e2ca2b206ca\
        e9700fa1825dbf55619f4ca7b3d52a94b36d2866ce58ee40ede45ccda62bd91aa89a536e288f91667\
        3a00afe1d270593783c5005fc2ca5398a64474424e111ec0f11721cac67bda316b085f05b744236b9\
        7e5262a1186f80ea40fc877c3edf6b71431d29541135e000bdfe34304b36f6cc316d79a03ff3b6d90\
        f282d5c581504";
    assert_eq!(expected, hex::encode(brotli_compressed_msgpack));

    let json_serialized_result = serde_json::to_string_pretty(&config);
    assert!(json_serialized_result.is_ok());
    let json_serialized = json_serialized_result.unwrap();
    let expected = serde_json::json!({
      "input": {
        "type": "ingestor",
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
        "metadata_ids": null,
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
