use std::io::Write;

use anyhow::Result;
use brotli::CompressorWriter;
use serde::Serialize;

use crate::clp_config::S3Config;

/// Represents CLP IO config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct ClpIoConfig {
    pub input: InputConfig,
    pub output: OutputConfig,
}

impl ClpIoConfig {
    /// Serializes the given [`ClpIoConfig`] instance to a Brotli-compressed MessagePack-encoded
    /// byte sequence.
    ///
    /// # Return
    /// A vector of bytes containing the serialized byte sequence.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`rmp_serde::to_vec_named`]'s errors on failure.
    /// * Forwards [`std::io::Write::write_all`]'s errors on failure.
    pub fn to_brotli_compressed_msgpack(&self) -> Result<Vec<u8>> {
        let msgpack_data = rmp_serde::to_vec_named(self)?;
        let mut brotli_compressor = CompressorWriter::new(Vec::new(), 4096, 5, 22);
        brotli_compressor.write_all(&msgpack_data)?;
        Ok(brotli_compressor.into_inner())
    }
}

// An enum representing CLP input config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(tag = "type")]
pub enum InputConfig {
    #[serde(rename = "s3")]
    S3InputConfig {
        #[serde(flatten)]
        config: S3InputConfig,
    },
}

/// Represents S3 input config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct S3InputConfig {
    #[serde(flatten)]
    pub s3_config: S3Config,

    pub keys: Option<Vec<String>>,
    pub dataset: Option<String>,
    pub timestamp_key: Option<String>,
    pub unstructured: bool,
}

/// Represents CLP output config.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct OutputConfig {
    pub tags: Option<Vec<String>>,
    pub target_archive_size: u64,
    pub target_dictionaries_size: u64,
    pub target_encoded_file_size: u64,
    pub target_segment_size: u64,
    pub compression_level: u8,
}

#[cfg(test)]
mod tests {
    use serde_json::Value;

    use super::*;
    use crate::clp_config::{
        AwsAuthentication,
        AwsCredentials,
        S3Config,
        scheduler::S3InputConfig,
    };

    #[test]
    fn test_serialization() {
        let s3_config = S3Config {
            bucket: "yscope".into(),
            region_code: "us-east-2".into(),
            key_prefix: "sample-logs/cockroachdb.clp.zst".into(),
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
                    dataset: Some("test-dataset".into()),
                    timestamp_key: Some("timestamp".into()),
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

        let brotli_compressed_msgpack_result = config.to_brotli_compressed_msgpack();
        assert!(brotli_compressed_msgpack_result.is_ok());
        let brotli_compressed_msgpack = brotli_compressed_msgpack_result.unwrap();
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
        let expected_json = r#"{
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
    "target_archive_size": 268435456,
    "target_dictionaries_size": 33554432,
    "target_encoded_file_size": 268435456,
    "target_segment_size": 268435456,
    "compression_level": 3
  }
}"#;
        let actual: Value = serde_json::from_str(json_serialized.as_str())
            .expect("The serialization result should be a valid JSON string.");
        let expected: Value = serde_json::from_str(expected_json)
            .expect("The expected serialization result should be a valid JSON string.");
        assert_eq!(expected, actual);
    }
}
