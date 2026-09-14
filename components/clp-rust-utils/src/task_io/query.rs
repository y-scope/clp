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

    /// The per-archive result limit. When absent, the task omits `--max-num-results` and uses the
    /// `clp-s` default.
    pub max_num_results: Option<NonZeroU32>,

    /// Inclusive `--tge` bound in Unix epoch milliseconds.
    pub begin_timestamp_millisecs: Option<i64>,

    /// Inclusive `--tle` bound in Unix epoch milliseconds.
    pub end_timestamp_millisecs: Option<i64>,

    /// Whether `clp-s` performs a case-insensitive search.
    pub ignore_case: bool,
}

/// The output handler that `clp-s` writes a query task's results to.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
#[serde(deny_unknown_fields, tag = "type")]
pub enum OutputHandle {
    /// The results cache, addressed by a MongoDB URI whose path names the database. The collection
    /// is the query job's ID.
    #[serde(rename = "results_cache")]
    ResultsCache { uri: NonEmptyString },

    /// A file per archive. Not yet supported by the Spider query flow.
    #[serde(rename = "file")]
    File,
}
