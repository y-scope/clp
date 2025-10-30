use num_enum::{IntoPrimitive, TryFromPrimitive};
use serde::{Deserialize, Serialize};

pub const QUERY_JOBS_TABLE_NAME: &str = "query_jobs";

// TODO: Add definition.
pub type AggregationConfig = ();

#[derive(Default, Serialize, Deserialize)]
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
    pub aggregation_config: Option<AggregationConfig>,
}

#[derive(Debug, Serialize, Deserialize, IntoPrimitive, TryFromPrimitive)]
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

#[derive(Debug, Serialize, Deserialize, IntoPrimitive, TryFromPrimitive)]
#[repr(i32)]
pub enum QueryJobType {
    SearchOrAggregation = 0,
    ExtractIr = 1,
    ExtractJson = 2,
}
