//! Protocol types for the CLP extraction task.

use std::num::NonZeroU64;

use serde::Deserialize;
use serde::Serialize;

use crate::dataset::DatasetName;
use crate::job_config::QueryJobId;
use crate::types::ArchiveId;

/// Input for extracting one archive into ordered JSON chunks.
#[derive(Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ExtractTaskInput {
    pub job_id: QueryJobId,
    pub dataset: DatasetName,
    pub archive_id: ArchiveId,
    pub target_chunk_size: Option<NonZeroU64>,
}
