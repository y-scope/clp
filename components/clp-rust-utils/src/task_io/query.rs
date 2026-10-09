//! Protocol types exchanged with the Spider (Huntsman) tasks that run CLP query jobs.

use std::num::NonZeroU32;
use std::num::NonZeroU64;

use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

use crate::job_config::SearchJobConfig;

/// `clp-s` options for a query job.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ClpSQueryOption {
    /// The query string passed positionally to `clp-s`.
    pub query_string: NonEmptyString,

    /// The per-archive raw-result limit. When absent, the task omits `--max-num-results` and uses
    /// the `clp-s` default. Timeline queries omit this limit to count every matching record.
    pub max_num_results: Option<NonZeroU32>,

    /// Inclusive `--tge` bound in Unix epoch milliseconds.
    pub begin_timestamp_millisecs: Option<i64>,

    /// Inclusive `--tle` bound in Unix epoch milliseconds.
    pub end_timestamp_millisecs: Option<i64>,

    /// Whether `clp-s` performs a case-insensitive search.
    pub ignore_case: bool,

    /// The positive count-by-time bucket width in milliseconds.
    ///
    /// * When set, the query aggregates the timestamps of all log events in the archive.
    /// * When absent, the query returns matching records without count-by-time aggregation.
    pub count_by_time_bucket_size_millisecs: Option<NonZeroU64>,
}

impl TryFrom<&SearchJobConfig> for ClpSQueryOption {
    type Error = QueryOptionError;

    fn try_from(config: &SearchJobConfig) -> Result<Self, Self::Error> {
        let bucket_size = config
            .aggregation_config
            .as_ref()
            .and_then(|aggregation| aggregation.count_by_time_bucket_size)
            .map(|bucket_size| {
                u64::try_from(bucket_size)
                    .ok()
                    .and_then(NonZeroU64::new)
                    .ok_or(QueryOptionError::InvalidCountByTimeBucketSize { bucket_size })
            })
            .transpose()?;
        Ok(Self {
            query_string: NonEmptyString::try_from(config.query_string.clone())
                .map_err(|_| QueryOptionError::EmptyQuery)?,
            max_num_results: if bucket_size.is_some() {
                None
            } else {
                NonZeroU32::new(config.max_num_results)
            },
            begin_timestamp_millisecs: config.begin_timestamp,
            end_timestamp_millisecs: config.end_timestamp,
            ignore_case: config.ignore_case,
            count_by_time_bucket_size_millisecs: bucket_size,
        })
    }
}

/// Invalid options in a search job configuration.
#[derive(Debug, Eq, PartialEq, thiserror::Error)]
pub enum QueryOptionError {
    #[error("query string must not be empty")]
    EmptyQuery,

    #[error("count-by-time bucket size must be positive, got {bucket_size}")]
    InvalidCountByTimeBucketSize { bucket_size: i64 },
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
    use std::num::NonZeroU64;

    use non_empty_string::NonEmptyString;
    use serde::Deserialize;
    use serde::Serialize;

    use super::ClpSQueryOption;
    use super::OutputHandle;
    use super::QueryOptionError;
    use crate::job_config::QueryJobId;
    use crate::types::non_empty_string::ExpectedNonEmpty;

    /// Identifies the results produced by a timeline query task.
    #[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
    #[serde(deny_unknown_fields)]
    struct TimelineTaskOutput {
        query_job_id: QueryJobId,
        output_handle: OutputHandle,
    }

    #[test]
    fn clp_s_query_option_with_timestamp_bounds_round_trips_through_msgpack() {
        let expected = ClpSQueryOption {
            query_string: NonEmptyString::from_static_str("level:error"),
            max_num_results: Some(NonZeroU32::new(1_000).expect("1,000 is nonzero")),
            begin_timestamp_millisecs: Some(1_700_000_000_001),
            end_timestamp_millisecs: Some(1_700_000_000_999),
            ignore_case: true,
            count_by_time_bucket_size_millisecs: None,
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
            count_by_time_bucket_size_millisecs: None,
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
            count_by_time_bucket_size_millisecs: None,
        };

        let serialized = rmp_serde::to_vec(&expected).expect("query options should serialize");
        let actual: ClpSQueryOption =
            rmp_serde::from_slice(&serialized).expect("query options should deserialize");

        assert_eq!(expected, actual);
    }

    #[test]
    fn invalid_aggregation_config_is_rejected() {
        use crate::job_config::AggregationConfig;
        use crate::job_config::SearchJobConfig;

        let base = SearchJobConfig {
            query_string: "*".to_owned(),
            max_num_results: 7,
            ..Default::default()
        };
        assert_eq!(
            ClpSQueryOption::try_from(&base)
                .expect("valid search configuration should convert")
                .max_num_results
                .map(NonZeroU32::get),
            Some(7)
        );
        for (aggregation, expected_error) in [
            (
                AggregationConfig {
                    count_by_time_bucket_size: Some(0),
                    ..Default::default()
                },
                QueryOptionError::InvalidCountByTimeBucketSize { bucket_size: 0 },
            ),
            (
                AggregationConfig {
                    count_by_time_bucket_size: Some(-1),
                    ..Default::default()
                },
                QueryOptionError::InvalidCountByTimeBucketSize { bucket_size: -1 },
            ),
        ] {
            let config = SearchJobConfig {
                aggregation_config: Some(aggregation),
                ..base.clone()
            };
            assert_eq!(ClpSQueryOption::try_from(&config), Err(expected_error));
        }
    }

    #[test]
    fn empty_query_is_rejected() {
        let config = crate::job_config::SearchJobConfig::default();
        assert_eq!(
            ClpSQueryOption::try_from(&config),
            Err(QueryOptionError::EmptyQuery)
        );
    }

    #[test]
    fn ordinary_search_preserves_aggregation_and_timestamp_behavior() {
        let config = crate::job_config::SearchJobConfig {
            query_string: "*".to_owned(),
            max_num_results: 7,
            begin_timestamp: Some(1),
            end_timestamp: Some(0),
            aggregation_config: Some(crate::job_config::AggregationConfig {
                do_count_aggregation: Some(true),
                ..Default::default()
            }),
            ..Default::default()
        };
        let actual =
            ClpSQueryOption::try_from(&config).expect("valid search configuration should convert");
        assert_eq!(actual.count_by_time_bucket_size_millisecs, None);
        assert_eq!(
            actual.max_num_results.map(NonZeroU32::get),
            Some(config.max_num_results)
        );
        assert_eq!(actual.begin_timestamp_millisecs, config.begin_timestamp);
        assert_eq!(actual.end_timestamp_millisecs, config.end_timestamp);
    }

    #[test]
    fn persisted_search_config_converts_supported_options() {
        for width in [None, Some(1_i64), Some(i64::MAX)] {
            for max_num_results in [0_u32, 7] {
                for (begin, end) in [
                    (None, None),
                    (Some(-1), None),
                    (None, Some(0)),
                    (Some(1), Some(1)),
                    (Some(-1), Some(1)),
                ] {
                    let persisted = serde_json::json!({
                        "datasets": ["logs"],
                        "query_string": "level:error",
                        "max_num_results": max_num_results,
                        "begin_timestamp": begin,
                        "end_timestamp": end,
                        "ignore_case": true,
                        "aggregation_config": width.map(|value| serde_json::json!({
                            "count_by_time_bucket_size": value,
                            "do_count_aggregation": false,
                        })),
                    });
                    let serialized =
                        rmp_serde::to_vec(&persisted).expect("test payload should serialize");
                    let config: crate::job_config::SearchJobConfig =
                        rmp_serde::from_slice(&serialized)
                            .expect("serialized test payload should deserialize");
                    let actual = ClpSQueryOption::try_from(&config)
                        .expect("valid search configuration should convert");
                    let expected = ClpSQueryOption {
                        query_string: NonEmptyString::from_static_str("level:error"),
                        max_num_results: if width.is_some() {
                            None
                        } else {
                            NonZeroU32::new(max_num_results)
                        },
                        begin_timestamp_millisecs: begin,
                        end_timestamp_millisecs: end,
                        ignore_case: true,
                        count_by_time_bucket_size_millisecs: width.map(|value| {
                            NonZeroU64::new(u64::try_from(value).expect("positive width"))
                                .expect("nonzero width")
                        }),
                    };
                    assert_eq!(actual, expected);
                }
            }
        }
    }

    #[test]
    fn timeline_bucket_width_round_trips_through_msgpack() {
        for width in [None, NonZeroU64::new(1), NonZeroU64::new(60_000)] {
            let expected = ClpSQueryOption {
                query_string: NonEmptyString::from_static_str("*"),
                max_num_results: None,
                begin_timestamp_millisecs: None,
                end_timestamp_millisecs: None,
                ignore_case: false,
                count_by_time_bucket_size_millisecs: width,
            };
            for serialized in [
                rmp_serde::to_vec(&expected).expect("test payload should serialize"),
                rmp_serde::to_vec_named(&expected).expect("test payload should serialize"),
            ] {
                let actual: ClpSQueryOption = rmp_serde::from_slice(&serialized)
                    .expect("serialized test payload should deserialize");
                assert_eq!(actual, expected);
            }
        }
    }

    #[test]
    fn timeline_bucket_width_rejects_nonpositive_values() {
        for width in [0_i64, -1] {
            let map = serde_json::json!({
                "query_string": "*",
                "max_num_results": null,
                "begin_timestamp_millisecs": null,
                "end_timestamp_millisecs": null,
                "ignore_case": false,
                "count_by_time_bucket_size_millisecs": width,
            });
            let sequence = ("*", None::<u32>, None::<i64>, None::<i64>, false, width);
            for serialized in [
                rmp_serde::to_vec(&map).expect("test payload should serialize"),
                rmp_serde::to_vec(&sequence).expect("test payload should serialize"),
            ] {
                assert!(rmp_serde::from_slice::<ClpSQueryOption>(&serialized).is_err());
            }
        }
    }

    #[test]
    fn timeline_task_output_preserves_msgpack_contract() {
        let expected = TimelineTaskOutput {
            query_job_id: 42,
            output_handle: OutputHandle::ResultsCache {
                uri: NonEmptyString::from_static_str("mongodb://results-cache:27017/timeline"),
            },
        };
        let expected_handle = serde_json::json!({
            "type": "results_cache",
            "uri": "mongodb://results-cache:27017/timeline",
        });
        let serialized = rmp_serde::to_vec(&expected).expect("test payload should serialize");
        let contract: (i32, serde_json::Value) =
            rmp_serde::from_slice(&serialized).expect("serialized test payload should deserialize");
        assert_eq!(
            contract,
            (
                42,
                serde_json::json!(["results_cache", "mongodb://results-cache:27017/timeline"])
            )
        );
        let actual: TimelineTaskOutput =
            rmp_serde::from_slice(&serialized).expect("serialized test payload should deserialize");
        assert_eq!(actual, expected);

        let named_contract = serde_json::json!({
            "query_job_id": 42,
            "output_handle": expected_handle,
        });
        let serialized = rmp_serde::to_vec_named(&expected).expect("test payload should serialize");
        let actual_contract: serde_json::Value =
            rmp_serde::from_slice(&serialized).expect("serialized test payload should deserialize");
        assert_eq!(actual_contract, named_contract);
        let actual: TimelineTaskOutput =
            rmp_serde::from_slice(&serialized).expect("serialized test payload should deserialize");
        assert_eq!(actual, expected);
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
