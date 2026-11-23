//! Protocol types for the CLP extraction task.

use std::num::NonZeroU64;

use serde::Deserialize;
use serde::Deserializer;
use serde::Serialize;

use crate::dataset::DatasetName;
use crate::job_config::QueryJobId;
use crate::types::ArchiveId;

/// Input for extracting one archive into ordered JSON chunks.
#[derive(Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ExtractTaskInput {
    pub job_id: QueryJobId,
    #[serde(deserialize_with = "deserialize_required_dataset")]
    pub dataset: DatasetName,
    pub archive_id: ArchiveId,
    pub target_chunk_size: Option<NonZeroU64>,
}

/// Deserializes a dataset name from a required string.
///
/// # Type Parameters
///
/// * `'de` - The lifetime of the input data.
/// * `DeserializerType` - The type of the deserializer to read the dataset name from.
///
/// # Returns
///
/// The validated dataset name on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Deserializer::Error`] created by [`serde::de::Error::custom`] if the name is not valid.
/// * Forwards [`String::deserialize`]'s return values on failure.
fn deserialize_required_dataset<'de, DeserializerType: Deserializer<'de>>(
    deserializer: DeserializerType,
) -> Result<DatasetName, DeserializerType::Error> {
    let name = String::deserialize(deserializer)?;
    DatasetName::try_from(name).map_err(serde::de::Error::custom)
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroU64;

    use super::ExtractTaskInput;
    use crate::dataset::DatasetName;
    use crate::types::ArchiveId;

    #[test]
    fn extract_task_input_requires_an_explicit_dataset() {
        let archive_id = "018e90e5-8b2a-4a61-a2fc-cac799936caf"
            .parse::<ArchiveId>()
            .expect("the fixture contains a valid archive ID");
        let task_input = |dataset: Option<&str>| {
            rmp_serde::to_vec(&(42, dataset, archive_id, None::<NonZeroU64>))
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
