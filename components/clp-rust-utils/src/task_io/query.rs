//! Protocol types exchanged with the Spider (Huntsman) tasks that run CLP query jobs.

use std::num::NonZeroU32;

use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

/// `clp-s` options for a query job.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
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

#[cfg(test)]
mod tests {
    use std::num::NonZeroU32;

    use non_empty_string::NonEmptyString;

    use super::ClpSQueryOption;
    use super::OutputHandle;
    use crate::types::non_empty_string::ExpectedNonEmpty;

    #[test]
    fn clp_s_query_option_with_timestamp_bounds_round_trips_through_msgpack() {
        let expected = ClpSQueryOption {
            query_string: NonEmptyString::from_static_str("level:error"),
            max_num_results: Some(NonZeroU32::new(1_000).expect("1,000 is nonzero")),
            begin_timestamp_millisecs: Some(1_700_000_000_001),
            end_timestamp_millisecs: Some(1_700_000_000_999),
            ignore_case: true,
        };

        let serialized = rmp_serde::to_vec(&expected).expect("query options should serialize");
        let actual: ClpSQueryOption =
            rmp_serde::from_slice(&serialized).expect("query options should deserialize");

        assert_eq!(expected, actual);
    }

    #[test]
    fn clp_s_query_option_without_timestamp_bounds_round_trips_through_msgpack() {
        let expected = ClpSQueryOption {
            query_string: NonEmptyString::from_static_str("*"),
            max_num_results: Some(NonZeroU32::new(1).expect("1 is nonzero")),
            begin_timestamp_millisecs: None,
            end_timestamp_millisecs: None,
            ignore_case: false,
        };

        let serialized = rmp_serde::to_vec(&expected).expect("query options should serialize");
        let actual: ClpSQueryOption =
            rmp_serde::from_slice(&serialized).expect("query options should deserialize");

        assert_eq!(expected, actual);
    }

    #[test]
    fn clp_s_query_option_without_max_num_results_round_trips_through_msgpack() {
        let expected = ClpSQueryOption {
            query_string: NonEmptyString::from_static_str("*"),
            max_num_results: None,
            begin_timestamp_millisecs: None,
            end_timestamp_millisecs: None,
            ignore_case: false,
        };

        let serialized = rmp_serde::to_vec(&expected).expect("query options should serialize");
        let actual: ClpSQueryOption =
            rmp_serde::from_slice(&serialized).expect("query options should deserialize");

        assert_eq!(expected, actual);
    }

    #[test]
    fn output_handle_results_cache_round_trips_through_msgpack() {
        let expected = OutputHandle::ResultsCache {
            uri: NonEmptyString::from_static_str("mongodb://results-cache:27017/clp-query-results"),
        };

        let serialized = rmp_serde::to_vec(&expected).expect("output handle should serialize");
        let actual: OutputHandle =
            rmp_serde::from_slice(&serialized).expect("output handle should deserialize");

        assert_eq!(expected, actual);
    }

    #[test]
    fn output_handle_file_round_trips_through_msgpack() {
        let expected = OutputHandle::File;

        let serialized = rmp_serde::to_vec(&expected).expect("output handle should serialize");
        let actual: OutputHandle =
            rmp_serde::from_slice(&serialized).expect("output handle should deserialize");

        assert_eq!(expected, actual);
    }
}
