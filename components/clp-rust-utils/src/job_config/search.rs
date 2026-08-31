use num_enum::IntoPrimitive;
use num_enum::TryFromPrimitive;
use serde::Deserialize;
use serde::Serialize;

pub type QueryJobId = i32;

pub const QUERY_JOBS_TABLE_NAME: &str = "query_jobs";

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
    pub datasets: Option<Vec<String>>,
    pub query_string: String,
    pub max_num_results: u32,
    pub begin_timestamp: Option<i64>,
    pub end_timestamp: Option<i64>,
    pub ignore_case: bool,
    pub path_filter: Option<String>,
    pub network_address: Option<(String, u16)>,
    pub aggregation_config: Option<AggregationConfig>,
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
