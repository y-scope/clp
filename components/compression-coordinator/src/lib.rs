//! The compression-job-submission API trait for driving CLP S3 compression jobs on a Spider
//! (Huntsman) cluster.

pub mod compression_job_submitter;
mod error;

pub use compression_job_submitter::{CompressionJobOutcome, S3CompressionJobSubmitter};
pub use error::Error;
