//! Protocol types exchanged with the Spider (Huntsman) tasks that run CLP query jobs.

use std::num::NonZeroU32;

use serde::Deserialize;
use serde::Serialize;

/// `clp-s` options for a query job.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct ClpSQueryOption {
    /// The query string passed positionally to `clp-s`.
    pub query_string: String,

    /// The maximum number of results retained by one archive-query invocation.
    pub max_num_results: NonZeroU32,

    /// Inclusive `--tge` bound in Unix epoch milliseconds.
    pub begin_timestamp_millisecs: Option<i64>,

    /// Inclusive `--tle` bound in Unix epoch milliseconds.
    pub end_timestamp_millisecs: Option<i64>,

    /// Whether `clp-s` performs a case-insensitive search.
    pub ignore_case: bool,
}

/// The graph output of one successfully completed archive-query task.
///
/// Query results are written directly to the results cache and are not returned through Spider.
/// This output only identifies the archive whose query invocation completed; it does not
/// finalize the query job.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct QueryTaskOutput {
    /// The resolved dataset containing the queried archive.
    pub dataset: String,

    /// The identifier of the queried archive.
    pub archive_id: String,
}
