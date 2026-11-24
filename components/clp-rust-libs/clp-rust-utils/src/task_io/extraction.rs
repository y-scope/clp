//! Protocol types for the CLP extraction task.

use std::num::NonZeroU64;

use serde::Deserialize;
use serde::Serialize;
use serde_with::DisplayFromStr;
use serde_with::serde_as;

use crate::dataset::DatasetName;
use crate::job_config::QueryJobId;
use crate::types::ArchiveUuid;

/// Input for extracting one archive into ordered JSON chunks.
#[serde_as]
#[derive(Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ExtractTaskInput {
    pub job_id: QueryJobId,
    #[serde_as(deserialize_as = "DisplayFromStr")]
    pub dataset: DatasetName,
    pub archive_uuid: ArchiveUuid,
    pub target_chunk_size: Option<NonZeroU64>,
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroU64;

    use super::ExtractTaskInput;
    use crate::dataset::DatasetName;
    use crate::types::ArchiveUuid;

    #[test]
    fn extract_task_input_requires_an_explicit_dataset() {
        let archive_uuid = "018e90e5-8b2a-4a61-a2fc-cac799936caf"
            .parse::<ArchiveUuid>()
            .expect("the fixture contains a valid archive UUID");
        let task_input = |dataset: Option<&str>| {
            rmp_serde::to_vec(&(42, dataset, archive_uuid, None::<NonZeroU64>))
                .expect("the positional task fixture should serialize")
        };

        let explicit_default = task_input(Some(DatasetName::default_name()));
        let input = rmp_serde::from_slice::<ExtractTaskInput>(&explicit_default)
            .expect("an explicit default dataset should be accepted");
        assert_eq!(input.dataset, DatasetName::default());

        rmp_serde::from_slice::<ExtractTaskInput>(&task_input(None))
            .expect_err("a null dataset should be rejected");
    }
}
