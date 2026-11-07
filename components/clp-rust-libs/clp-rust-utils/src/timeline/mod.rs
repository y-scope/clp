//! Aggregation and validation of count-by-time results.

use crate::job_config::QueryJobId;

mod aggregation;
mod bucket;
mod error;

pub use aggregation::merge_archive_buckets;
pub use bucket::Bucket;
pub use error::Error;

/// Reserved document identity indicating that all final buckets have been published.
pub const COMMIT_MARKER_ID: &str = "__clp_timeline_committed";

/// # Returns
///
/// The results-cache collection name for a query job's timeline.
#[must_use]
pub fn collection_name(query_job_id: QueryJobId) -> String {
    format!("{query_job_id}-timeline")
}
