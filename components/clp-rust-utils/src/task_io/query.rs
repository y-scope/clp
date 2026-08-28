//! Protocol types exchanged with the Spider (Huntsman) tasks that run CLP query jobs.

use std::num::NonZeroU32;

use serde::Deserialize;
use serde::Serialize;

/// `clp-s` tuning and engine options for a query job.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct ClpSQueryOption {
    /// The query string passed positionally to `clp-s`.
    pub query_string: String,

    /// The maximum number of results retained by one archive-query invocation.
    pub max_num_results: NonZeroU32,

    /// The inclusive lower timestamp bound (`--tge`), in Unix epoch microseconds.
    pub begin_timestamp: Option<i64>,

    /// The inclusive upper timestamp bound (`--tle`), in Unix epoch microseconds.
    pub end_timestamp: Option<i64>,

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

#[cfg(test)]
mod tests {
    use std::num::NonZeroU32;

    use super::ClpSQueryOption;
    use super::QueryTaskOutput;

    #[test]
    fn clp_s_query_option_with_timestamp_bounds_round_trips_through_msgpack() {
        let expected = ClpSQueryOption {
            query_string: "level:error".to_owned(),
            max_num_results: NonZeroU32::new(1_000).expect("1,000 is nonzero"),
            begin_timestamp: Some(1_700_000_000_000_001),
            end_timestamp: Some(1_700_000_000_000_999),
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
            query_string: "*".to_owned(),
            max_num_results: NonZeroU32::new(1).expect("1 is nonzero"),
            begin_timestamp: None,
            end_timestamp: None,
            ignore_case: false,
        };

        let serialized = rmp_serde::to_vec(&expected).expect("query options should serialize");
        let actual: ClpSQueryOption =
            rmp_serde::from_slice(&serialized).expect("query options should deserialize");

        assert_eq!(expected, actual);
    }

    #[test]
    fn query_task_output_round_trips_through_msgpack() {
        let expected = QueryTaskOutput {
            dataset: "default".to_owned(),
            archive_id: "archive-id".to_owned(),
        };

        let serialized = rmp_serde::to_vec(&expected).expect("task output should serialize");
        let actual: QueryTaskOutput =
            rmp_serde::from_slice(&serialized).expect("task output should deserialize");

        assert_eq!(expected, actual);
    }
}
