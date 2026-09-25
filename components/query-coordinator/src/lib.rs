//! Coordination for CLP query jobs.

pub mod archive_selection;
mod error;
pub mod job_handle;
pub mod query_job_submitter;

pub use error::Error;
