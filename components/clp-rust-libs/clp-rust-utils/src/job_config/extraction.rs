//! Configs of stream extraction jobs.

use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;
use serde_with::DisplayFromStr;
use serde_with::serde_as;

use crate::dataset::DatasetName;
use crate::types::ArchiveUuid;

/// Config of a job that extracts the IR stream of an original file containing a given log event.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct ExtractIrJobConfig {
    pub orig_file_id: NonEmptyString,
    pub msg_ix: u64,
    pub file_split_id: Option<NonEmptyString>,
    pub target_uncompressed_size: Option<u64>,
}

/// Config of a job that extracts the JSON stream of an archive.
#[serde_as]
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct ExtractJsonJobConfig {
    pub dataset: DatasetName,
    // The extraction job scheduler reads the archive's UUID as `archive_id`.
    #[serde(rename = "archive_id")]
    #[serde_as(as = "DisplayFromStr")]
    pub archive_uuid: ArchiveUuid,
    pub target_chunk_size: Option<u64>,
}
