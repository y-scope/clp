use std::num::NonZeroU32;

use num_enum::IntoPrimitive;
use num_enum::TryFromPrimitive;
use serde::Deserialize;
use serde::Serialize;
use strum::EnumString;
use utoipa::ToSchema;

use crate::dataset::DatasetName;

pub const QUERY_JOBS_TABLE_NAME: &str = "query_jobs";

pub type QueryJobId = i32;

/// Mirror of `job_orchestration.scheduler.job_config.AggregationConfig`. Must be kept in sync.
#[derive(Clone, Debug, Default, Deserialize, Eq, PartialEq, Serialize)]
#[serde(default)]
pub struct AggregationConfig {
    pub job_id: Option<i64>,
    pub reducer_host: Option<String>,
    pub reducer_port: Option<u16>,
    pub do_count_aggregation: Option<bool>,
    /// Milliseconds
    pub count_by_time_bucket_size: Option<i64>,
}

/// Mirror of `job_orchestration.scheduler.job_config.SearchJobConfig`. Must be kept in sync.
#[derive(Clone, Debug, Default, Deserialize, Eq, PartialEq, Serialize)]
#[serde(default)]
pub struct SearchJobConfig {
    pub datasets: Option<Vec<DatasetName>>,
    pub query_string: String,
    pub max_num_results: Option<NonZeroU32>,
    pub begin_timestamp: Option<i64>,
    pub end_timestamp: Option<i64>,
    pub ignore_case: bool,
    pub path_filter: Option<String>,
    pub network_address: Option<(String, u16)>,
    pub aggregation_config: Option<AggregationConfig>,
    pub write_to_file: bool,
}

/// Mirror of `job_orchestration.scheduler.constants.QueryJobStatus`. Must be kept in sync.
#[derive(
    Clone,
    Copy,
    Debug,
    Deserialize,
    EnumString,
    Eq,
    IntoPrimitive,
    PartialEq,
    Serialize,
    ToSchema,
    TryFromPrimitive,
    sqlx::Type,
)]
#[repr(i32)]
#[strum(ascii_case_insensitive)]
pub enum QueryJobStatus {
    Pending = 0,
    Running = 1,
    Succeeded = 2,
    Failed = 3,
    CancelRequested = 4,
    Cancelling = 5,
    Cancelled = 6,
    Killed = 7,
}

impl QueryJobStatus {
    /// # Returns
    ///
    /// Whether the status is terminal, i.e., whether the job has finished and its status will no
    /// longer change.
    #[must_use]
    pub const fn is_terminal(self) -> bool {
        matches!(
            self,
            Self::Succeeded | Self::Failed | Self::Cancelled | Self::Killed
        )
    }
}

/// Mirror of `job_orchestration.scheduler.constants.QueryJobType`. Must be kept in sync.
#[derive(
    Clone,
    Copy,
    Debug,
    Deserialize,
    Eq,
    IntoPrimitive,
    PartialEq,
    Serialize,
    TryFromPrimitive,
    sqlx::Type,
)]
#[repr(i32)]
pub enum QueryJobType {
    SearchOrAggregation = 0,
    ExtractIr = 1,
    ExtractJson = 2,
}

#[cfg(test)]
mod tests {
    use serde_json::json;

    use super::SearchJobConfig;
    use crate::dataset::DatasetName;

    #[test]
    fn search_job_config_datasets_are_validated() {
        let deserialize_datasets = |config| {
            serde_json::from_value::<SearchJobConfig>(config).map(|config| config.datasets)
        };

        assert_eq!(
            deserialize_datasets(json!({}))
                .expect("search job config without datasets should deserialize"),
            None
        );
        assert_eq!(
            deserialize_datasets(json!({"datasets": null}))
                .expect("search job config with null datasets should deserialize"),
            None
        );
        assert_eq!(
            deserialize_datasets(json!({"datasets": ["default", "my_dataset"]}))
                .expect("search job config with valid datasets should deserialize"),
            Some(vec![
                DatasetName::default(),
                "my_dataset"
                    .parse()
                    .expect("name should be a valid dataset name"),
            ])
        );
        deserialize_datasets(json!({"datasets": ["default", "bad-name"]}))
            .expect_err("search job config with an invalid dataset should fail to deserialize");
    }
}
