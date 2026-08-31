//! Protocol types exchanged with the Spider (Huntsman) tasks that run CLP query jobs.

use std::num::NonZeroU32;

use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

/// `clp-s` options for a query job.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct ClpSQueryOption {
    /// The query string passed positionally to `clp-s`.
    pub query_string: NonEmptyString,

    /// The per-archive result limit. When absent,
    /// [`crate::clp_config::package::config::DEFAULT_MAX_NUM_QUERY_RESULTS`] is used.
    pub max_num_results: Option<NonZeroU32>,

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
    pub dataset: NonEmptyString,

    /// The identifier of the queried archive.
    pub archive_id: NonEmptyString,
}
