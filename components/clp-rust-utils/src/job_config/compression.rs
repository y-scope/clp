use num_enum::{IntoPrimitive, TryFromPrimitive};
use serde::{Deserialize, Serialize};
use strum::EnumString;
use utoipa::ToSchema;

pub type CompressionJobId = i32;

// Mirror of `job_orchestration.scheduler.constants.CompressionJobStatus`. Must be kept in sync.
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
)]
#[repr(i32)]
#[strum(ascii_case_insensitive)]
pub enum CompressionJobStatus {
    /// Job is waiting to be scheduled.
    Pending = 0,
    /// Job is currently executing.
    Running = 1,
    /// Job completed successfully.
    Succeeded = 2,
    /// Job failed.
    Failed = 3,
    /// Job was killed by a user.
    Killed = 4,
}
