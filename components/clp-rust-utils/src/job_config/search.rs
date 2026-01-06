use num_enum::{IntoPrimitive, TryFromPrimitive};
use serde::{Deserialize, Serialize};

pub const QUERY_JOBS_TABLE_NAME: &str = "query_jobs";

/// Mirror of `job_orchestration.scheduler.job_config.SearchJobConfig`. Must be kept in sync.
///
/// # NOTE
///
/// `aggregation_config` is currently unused and thus uses a placeholder unit type.
#[derive(Clone, Debug, Default, Deserialize, Eq, PartialEq, Serialize)]
#[serde(default)]
pub struct SearchJobConfig {
    pub dataset: Option<String>,
    pub query_string: String,
    pub max_num_results: u32,
    pub tags: Option<Vec<String>>,
    pub begin_timestamp: Option<i64>,
    pub end_timestamp: Option<i64>,
    pub ignore_case: bool,
    pub path_filter: Option<String>,
    pub network_address: Option<(String, u16)>,
    pub aggregation_config: Option<()>,
    pub write_to_file: bool,
}

/// Mirror of `job_orchestration.scheduler.constants.QueryJobStatus`. Must be kept in sync.
#[derive(Clone, Debug, Deserialize, Eq, IntoPrimitive, PartialEq, Serialize, TryFromPrimitive)]
#[repr(i32)]
pub enum QueryJobStatus {
    Pending = 0,
    Running = 1,
    Succeeded = 2,
    Failed = 3,
    Cancelling = 4,
    Cancelled = 5,
    Killed = 6,
}

/// Mirror of `job_orchestration.scheduler.constants.QueryJobType`. Must be kept in sync.
#[derive(Clone, Debug, Deserialize, Eq, IntoPrimitive, PartialEq, Serialize, TryFromPrimitive)]
#[repr(i32)]
pub enum QueryJobType {
    SearchOrAggregation = 0,
    ExtractIr = 1,
    ExtractJson = 2,
}
