//! Aggregation and validation of count-by-time results.

mod aggregation;
mod bucket;
mod error;

pub use aggregation::aggregate_source_contributions;
pub use bucket::Bucket;
pub use error::Error;

/// Reserved document identity indicating that all final buckets have been published.
pub const COMMIT_MARKER_ID: &str = "__clp_timeline_committed";
