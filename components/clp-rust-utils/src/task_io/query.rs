//! Protocol types exchanged with the Spider tasks that run CLP-S query jobs.

use std::num::NonZeroU32;

use serde::Deserialize;
use serde::Serialize;

/// The job-wide CLP-S options shared by every archive-search task in a query job.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct ClpSQueryOption {
    /// The query string passed positionally to `clp-s`.
    pub query_string: String,

    /// The maximum number of results retained by one archive-search invocation.
    pub max_num_results: NonZeroU32,

    /// The inclusive lower timestamp bound (`--tge`), in Unix epoch microseconds.
    pub begin_timestamp: Option<i64>,

    /// The inclusive upper timestamp bound (`--tle`), in Unix epoch microseconds.
    pub end_timestamp: Option<i64>,

    /// Whether `clp-s` performs a case-insensitive search.
    pub ignore_case: bool,
}

/// The graph output of one successfully completed archive-search task.
///
/// Search results are written directly to the results cache and are not returned through Spider.
/// This output only identifies the archive whose search invocation completed; it does not
/// finalize the query job.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct QueryTaskOutput {
    /// The resolved dataset containing the searched archive.
    pub dataset: String,

    /// The identifier of the searched archive.
    pub archive_id: String,
}
