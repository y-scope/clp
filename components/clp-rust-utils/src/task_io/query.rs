//! Protocol types exchanged with the Spider (Huntsman) tasks that run CLP query jobs.

use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

use crate::job_config::SearchQuery;

/// `clp-s` options for a query job.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ClpSQueryOption {
    /// The query string passed positionally to `clp-s`.
    pub query_string: NonEmptyString,

    /// Inclusive `--tge` bound in Unix epoch milliseconds.
    pub begin_timestamp_millisecs: Option<i64>,

    /// Inclusive `--tle` bound in Unix epoch milliseconds.
    pub end_timestamp_millisecs: Option<i64>,

    /// Whether `clp-s` performs a case-insensitive search.
    pub ignore_case: bool,
}

impl From<&SearchQuery> for ClpSQueryOption {
    fn from(query: &SearchQuery) -> Self {
        Self {
            query_string: query.query_string.clone(),
            begin_timestamp_millisecs: query.time_range.begin_timestamp_millisecs(),
            end_timestamp_millisecs: query.time_range.end_timestamp_millisecs(),
            ignore_case: query.ignore_case,
        }
    }
}

#[cfg(test)]
mod tests {
    use non_empty_string::NonEmptyString;

    use super::ClpSQueryOption;
    use crate::job_config::SearchQuery;
    use crate::job_config::TimeRange;
    use crate::types::non_empty_string::ExpectedNonEmpty;

    #[test]
    fn clp_s_query_option_with_timestamp_bounds_round_trips_through_msgpack() {
        let expected = ClpSQueryOption {
            query_string: NonEmptyString::from_static_str("level:error"),
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
    fn clp_s_query_option_from_search_query_copies_every_field() {
        let query = SearchQuery {
            query_string: NonEmptyString::from_static_str("level: ERROR"),
            time_range: TimeRange::new(Some(1), Some(2)).expect("range is ordered"),
            ignore_case: true,
        };

        assert_eq!(
            ClpSQueryOption::from(&query),
            ClpSQueryOption {
                query_string: NonEmptyString::from_static_str("level: ERROR"),
                begin_timestamp_millisecs: Some(1),
                end_timestamp_millisecs: Some(2),
                ignore_case: true,
            }
        );
    }
}
