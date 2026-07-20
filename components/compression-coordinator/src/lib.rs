//! Compression-job submission and S3 input partitioning for a Spider cluster.

pub mod compression_job_submitter;
mod error;
pub mod partition;

pub use error::Error;
