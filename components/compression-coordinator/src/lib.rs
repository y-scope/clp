//! Compression-job submission and S3 input partitioning for a Spider cluster.

pub mod compression_job_submitter;
pub mod coordination;
mod error;
pub mod job_handle;
pub mod partition;

pub use error::Error;
