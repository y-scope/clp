//! Configs of stream extraction jobs.

use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

use crate::dataset::DatasetName;
use crate::types::ArchiveId;

/// Config of a job that extracts the IR stream of an original file containing a given log event.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct ExtractIrJobConfig {
    pub orig_file_id: NonEmptyString,
    pub msg_ix: u64,
    pub file_split_id: Option<NonEmptyString>,
    pub target_uncompressed_size: Option<u64>,
}

/// Config of a job that extracts the JSON stream of an archive.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct ExtractJsonJobConfig {
    pub dataset: DatasetName,
    #[serde(with = "crate::serde::display_from_str")]
    pub archive_id: ArchiveId,
    pub target_chunk_size: Option<u64>,
}

#[cfg(test)]
mod tests {
    use non_empty_string::NonEmptyString;
    use serde_json::Value;
    use serde_json::json;

    use super::ExtractIrJobConfig;
    use super::ExtractJsonJobConfig;
    use crate::types::non_empty_string::ExpectedNonEmpty;

    #[test]
    fn extract_ir_job_config_round_trips_through_named_msgpack_with_expected_field_names() {
        let config = ExtractIrJobConfig {
            orig_file_id: NonEmptyString::from_static_str("orig-file-id"),
            msg_ix: 42,
            file_split_id: Some(NonEmptyString::from_static_str("file-split-id")),
            target_uncompressed_size: Some(128 * 1024 * 1024),
        };

        let serialized = rmp_serde::to_vec_named(&config).expect("job config should serialize");
        assert_eq!(
            rmp_serde::from_slice::<Value>(&serialized)
                .expect("serialized job config should decode as a map"),
            json!({
                "orig_file_id": config.orig_file_id.as_str(),
                "msg_ix": config.msg_ix,
                "file_split_id": config.file_split_id.as_ref().map(NonEmptyString::as_str),
                "target_uncompressed_size": config.target_uncompressed_size,
            })
        );
        assert_eq!(
            rmp_serde::from_slice::<ExtractIrJobConfig>(&serialized)
                .expect("job config should deserialize"),
            config
        );
    }

    #[test]
    fn extract_json_job_config_round_trips_through_named_msgpack_with_archive_id_as_string() {
        const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";
        let config = ExtractJsonJobConfig {
            dataset: "my_dataset"
                .parse()
                .expect("name should be a valid dataset name"),
            archive_id: ARCHIVE_ID.parse().expect("valid UUID should parse"),
            target_chunk_size: Some(64 * 1024 * 1024),
        };

        let serialized = rmp_serde::to_vec_named(&config).expect("job config should serialize");
        assert_eq!(
            rmp_serde::from_slice::<Value>(&serialized)
                .expect("serialized job config should decode as a map"),
            json!({
                "dataset": config.dataset.as_str(),
                "archive_id": ARCHIVE_ID,
                "target_chunk_size": config.target_chunk_size,
            })
        );
        assert_eq!(
            rmp_serde::from_slice::<ExtractJsonJobConfig>(&serialized)
                .expect("job config should deserialize"),
            config
        );
    }

    #[test]
    fn extract_json_job_config_deserializes_archive_id_strings_and_rejects_invalid_ones() {
        const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";
        let deserialize_archive_id =
            |archive_id: &str| -> Result<ExtractJsonJobConfig, rmp_serde::decode::Error> {
                let serialized = rmp_serde::to_vec_named(&json!({
                    "dataset": "my_dataset",
                    "archive_id": archive_id,
                    "target_chunk_size": null,
                }))
                .expect("job config should serialize");
                rmp_serde::from_slice(&serialized)
            };

        assert_eq!(
            deserialize_archive_id(ARCHIVE_ID)
                .expect("job config with a valid archive ID should deserialize"),
            ExtractJsonJobConfig {
                dataset: "my_dataset"
                    .parse()
                    .expect("name should be a valid dataset name"),
                archive_id: ARCHIVE_ID.parse().expect("valid UUID should parse"),
                target_chunk_size: None,
            }
        );
        deserialize_archive_id("not-a-uuid")
            .expect_err("job config with an invalid archive ID should fail to deserialize");
    }
}
