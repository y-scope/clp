//! Coordination for CLP query jobs.

mod cancellation_monitor;
pub mod coordination;
mod error;
mod job_handle;
mod spider_job_submitter;

pub use error::Error;
