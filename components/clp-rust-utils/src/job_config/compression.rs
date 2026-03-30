use num_enum::{IntoPrimitive, TryFromPrimitive};
use serde::{Deserialize, Serialize};

pub type CompressionJobId = i32;

/// Mirror of `job_orchestration.scheduler.constants.CompressionJobStatus`. Must be kept in sync.
#[derive(Clone, Debug, Deserialize, Eq, IntoPrimitive, PartialEq, Serialize, TryFromPrimitive)]
#[repr(i32)]
pub enum CompressionJobStatus {
    Pending = 0,
    Running = 1,
    Succeeded = 2,
    Failed = 3,
    Killed = 4,
}
