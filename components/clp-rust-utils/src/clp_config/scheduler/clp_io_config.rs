use anyhow::Result;
use brotli::CompressorWriter;
use serde::Serialize;

use crate::clp_config::scheduler::{InputConfig, OutputConfig};

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
    /// * Forwards [`std::io::copy`]'s errors on failure.
    pub fn to_brotli_compressed_msgpack(&self) -> Result<Vec<u8>> {
        let msgpack_data = rmp_serde::to_vec_named(self)?;
        let mut encoder = CompressorWriter::new(Vec::new(), 4096, 5, 22);
        std::io::copy(&mut &msgpack_data[..], &mut encoder)?;
        Ok(encoder.into_inner())
    }
}

#[cfg(test)]
mod tests {
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
            region: "us-east-2".into(),
            prefix: "sample-logs/cockroachdb.clp.zst".into(),
            aws_authentication: AwsAuthentication::Credentials {
                credentials: Some(AwsCredentials {
                    access_key_id: "ACCESS_KEY_ID".into(),
                    secret_access_key: "SECRET_ACCESS_KEY".into(),
                }),
            },
        };
        let config = ClpIoConfig {
            input: InputConfig::S3InputConfig {
                config: S3InputConfig {
                    s3_config,
                    keys: None,
                    dataset: Some("test-dataset".into()),
                    timestamp_key: Some("timestamp".into()),
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
        let expected = "1b890100e4f8fbb90019c91aa8c2a2e87ef6bd25ae8523c236a04dde621d4ef9ff68a30d2c\
                ff30d32cef02e9726ef277566e856e7449b9bf04e6e9c2f2f238c3e88a69b22a4f565d0acc2ac7d836d\
                17fb3ee8fac28335f51167ecd1431d80e1f1d9366258e09b09a15e821b9d16d04ba495a2332abaeee9f\
                92f7acd9e8c22032ade1bcf48f2cfd0495ef82cc1d638944aa548242aa01b9e495ae13c06eaea554a29\
                82a03542d8a2cffd6be9c35f2e4407824fe057fec0614eeb7c3ac444ce34599d5dffd97110b6cbe01ca\
                3dfeef73b95c9fcd83f7a3cb3c22600f7dbf3458430ef12c31ad59605e59531774caa601ca282f";
        assert_eq!(expected, hex::encode(brotli_compressed_msgpack));

        let json_serialized_result = serde_json::to_string_pretty(&config);
        assert!(json_serialized_result.is_ok());
        let json_serialized = json_serialized_result.unwrap();
        let expected_json = r#"{
  "input": {
    "type": "s3",
    "bucket": "yscope",
    "region": "us-east-2",
    "prefix": "sample-logs/cockroachdb.clp.zst",
    "aws_authentication": {
      "type": "credentials",
      "credentials": {
        "access_key_id": "ACCESS_KEY_ID",
        "secret_access_key": "SECRET_ACCESS_KEY"
      }
    },
    "keys": null,
    "dataset": "test-dataset",
    "timestamp_key": "timestamp"
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
        assert_eq!(expected_json, json_serialized);
    }
}
