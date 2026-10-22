//! Protocol types exchanged with the Spider (Huntsman) tasks that run CLP S3 compression jobs.

use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

use crate::clp_config::AwsAuthentication;
use crate::dataset::DatasetName;
use crate::types::ArchiveId;

/// `clp-s` tuning and engine options for a compression job.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ClpSCompressionOption {
    pub target_encoded_size: u64,
    pub compression_level: u8,
    pub timestamp_key: Option<String>,
    pub unstructured: bool,
}

/// Input source for a compression task.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct S3InputSource {
    pub endpoint_url: Option<NonEmptyString>,
    pub region_code: Option<NonEmptyString>,
    pub bucket: NonEmptyString,
    pub aws_authentication: AwsAuthentication,
    pub object_keys: Vec<String>,
}

/// Output of a compression task.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct CompressionTaskOutput {
    pub dataset: DatasetName,
    pub archives: Vec<ArchiveMetadata>,
}

/// Metadata of an archive produced by `clp-s`.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ArchiveMetadata {
    pub id: ArchiveId,
    pub begin_timestamp: i64,
    pub end_timestamp: i64,
    pub size: i64,
    pub uncompressed_size: i64,
}

#[cfg(test)]
mod tests {
    use serde::Serialize;

    use super::ArchiveMetadata;
    use super::CompressionTaskOutput;

    #[derive(Serialize)]
    struct UntypedCompressionTaskOutput {
        dataset: String,
        archives: Vec<ArchiveMetadata>,
    }

    #[test]
    fn compression_task_output_deserialization_validates_the_dataset() {
        let archives = vec![ArchiveMetadata {
            id: "018e90e5-8b2a-4a61-a2fc-cac799936caf"
                .parse()
                .expect("valid UUID should parse"),
            begin_timestamp: 0,
            end_timestamp: 1,
            size: 1,
            uncompressed_size: 1,
        }];

        let serialized = rmp_serde::to_vec(&UntypedCompressionTaskOutput {
            dataset: "my_dataset".to_owned(),
            archives: archives.clone(),
        })
        .expect("compression task output should serialize");
        let output: CompressionTaskOutput = rmp_serde::from_slice(&serialized)
            .expect("compression task output with a valid dataset should deserialize");
        assert_eq!(
            output,
            CompressionTaskOutput {
                dataset: "my_dataset"
                    .parse()
                    .expect("name should be a valid dataset name"),
                archives: archives.clone(),
            }
        );

        let serialized = rmp_serde::to_vec(&UntypedCompressionTaskOutput {
            dataset: "bad-name".to_owned(),
            archives,
        })
        .expect("compression task output should serialize");
        rmp_serde::from_slice::<CompressionTaskOutput>(&serialized).expect_err(
            "compression task output with an invalid dataset should fail to deserialize",
        );
    }
}
