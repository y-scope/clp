use std::num::NonZeroU16;
use std::num::NonZeroU32;

use non_empty_string::NonEmptyString;
use num_enum::IntoPrimitive;
use num_enum::TryFromPrimitive;
use serde::Deserialize;
use serde::Serialize;
use strum::EnumString;
use utoipa::ToSchema;

use crate::dataset::DatasetName;
use crate::types::SessionToken;

pub const QUERY_JOBS_TABLE_NAME: &str = "query_jobs";

pub const QUERY_TASKS_TABLE_NAME: &str = "query_tasks";

pub type QueryJobId = i32;

/// Mirror of `job_orchestration.scheduler.job_config.AggregationConfig`. Must be kept in sync.
#[derive(Clone, Debug, Default, Deserialize, Eq, PartialEq, Serialize)]
#[serde(default)]
pub struct AggregationConfig {
    pub job_id: Option<i64>,
    pub reducer_host: Option<String>,
    pub reducer_port: Option<u16>,
    pub do_count_aggregation: Option<bool>,
    /// Milliseconds
    pub count_by_time_bucket_size: Option<i64>,
}

/// The TCP listener that a query job's results are streamed to.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct NetworkOutput {
    pub host: NonEmptyString,
    pub port: NonZeroU16,
    pub session_token: SessionToken,
}

/// The configuration of a search or aggregation query job, as stored in the query jobs table.
#[derive(Clone, Debug, Default, Deserialize, Eq, PartialEq, Serialize)]
#[serde(default)]
pub struct SearchJobConfig {
    pub datasets: Option<Vec<DatasetName>>,
    pub query_string: String,
    pub max_num_results: Option<NonZeroU32>,
    pub begin_timestamp: Option<i64>,
    pub end_timestamp: Option<i64>,
    pub ignore_case: bool,
    pub path_filter: Option<String>,
    pub network_output: Option<NetworkOutput>,
    pub aggregation_config: Option<AggregationConfig>,
    pub write_to_file: bool,
}

impl SearchJobConfig {
    /// Checks that the config can be run as a query job.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`SearchJobConfigError::AggregationUnsupported`] if `aggregation_config` is set.
    /// * [`SearchJobConfigError::WriteToFileUnsupported`] if `write_to_file` is set.
    /// * [`SearchJobConfigError::MaxNumResultsUnsupported`] if both `network_output` and
    ///   `max_num_results` are set.
    /// * [`SearchJobConfigError::EmptyQuery`] if `query_string` is empty.
    /// * [`SearchJobConfigError::ReversedTimeRange`] if `begin_timestamp` is later than
    ///   `end_timestamp`.
    pub const fn validate(&self) -> Result<(), SearchJobConfigError> {
        if self.aggregation_config.is_some() {
            return Err(SearchJobConfigError::AggregationUnsupported);
        }
        if self.write_to_file {
            return Err(SearchJobConfigError::WriteToFileUnsupported);
        }
        if self.network_output.is_some() && self.max_num_results.is_some() {
            return Err(SearchJobConfigError::MaxNumResultsUnsupported);
        }
        if self.query_string.is_empty() {
            return Err(SearchJobConfigError::EmptyQuery);
        }
        if let (Some(begin_timestamp), Some(end_timestamp)) =
            (self.begin_timestamp, self.end_timestamp)
            && begin_timestamp > end_timestamp
        {
            return Err(SearchJobConfigError::ReversedTimeRange {
                begin_timestamp,
                end_timestamp,
            });
        }
        Ok(())
    }
}

/// Reasons a search job config can't be run as a query job.
#[derive(Debug, Eq, PartialEq, thiserror::Error)]
pub enum SearchJobConfigError {
    #[error("aggregation jobs are not supported")]
    AggregationUnsupported,

    #[error("writing query results to files is not supported")]
    WriteToFileUnsupported,

    #[error("max_num_results must be unset for a streaming query, which streams every result")]
    MaxNumResultsUnsupported,

    #[error("query string must not be empty")]
    EmptyQuery,

    #[error("begin timestamp {begin_timestamp} is greater than end timestamp {end_timestamp}")]
    ReversedTimeRange {
        begin_timestamp: i64,
        end_timestamp: i64,
    },
}

/// Mirror of `job_orchestration.scheduler.constants.QueryJobStatus`. Must be kept in sync.
#[derive(
    Clone,
    Copy,
    Debug,
    Deserialize,
    EnumString,
    Eq,
    IntoPrimitive,
    PartialEq,
    Serialize,
    ToSchema,
    TryFromPrimitive,
    sqlx::Type,
)]
#[repr(i32)]
#[strum(ascii_case_insensitive)]
pub enum QueryJobStatus {
    Pending = 0,
    Running = 1,
    Succeeded = 2,
    Failed = 3,
    CancelRequested = 4,
    Cancelling = 5,
    Cancelled = 6,
    Killed = 7,
}

impl QueryJobStatus {
    /// # Returns
    ///
    /// Whether the status is terminal, i.e., whether the job has finished and its status will no
    /// longer change.
    #[must_use]
    pub const fn is_terminal(self) -> bool {
        matches!(
            self,
            Self::Succeeded | Self::Failed | Self::Cancelled | Self::Killed
        )
    }
}

/// The cancellability of a query job, derived from its status.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Cancellability {
    Cancellable,
    CancellationRequested,
    NotCancellable,
}

impl Cancellability {
    /// # Returns
    ///
    /// The cancellability of a query job in the given status.
    #[must_use]
    pub const fn from_status(status: QueryJobStatus) -> Self {
        match status {
            QueryJobStatus::Pending | QueryJobStatus::Running => Self::Cancellable,
            QueryJobStatus::CancelRequested
            | QueryJobStatus::Cancelling
            | QueryJobStatus::Cancelled => Self::CancellationRequested,
            QueryJobStatus::Succeeded | QueryJobStatus::Failed | QueryJobStatus::Killed => {
                Self::NotCancellable
            }
        }
    }
}

/// Mirror of `job_orchestration.scheduler.constants.QueryJobType`. Must be kept in sync.
#[derive(
    Clone,
    Copy,
    Debug,
    Deserialize,
    Eq,
    IntoPrimitive,
    PartialEq,
    Serialize,
    TryFromPrimitive,
    sqlx::Type,
)]
#[repr(i32)]
pub enum QueryJobType {
    SearchOrAggregation = 0,
    ExtractIr = 1,
    ExtractJson = 2,
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroU16;
    use std::num::NonZeroU32;

    use non_empty_string::NonEmptyString;

    use super::AggregationConfig;
    use super::Cancellability;
    use super::NetworkOutput;
    use super::QueryJobStatus;
    use super::SearchJobConfig;
    use super::SearchJobConfigError;
    use crate::types::SessionToken;
    use crate::types::non_empty_string::ExpectedNonEmpty;

    /// # Returns
    ///
    /// A network output to a listener on `localhost`.
    fn network_output() -> NetworkOutput {
        NetworkOutput {
            host: NonEmptyString::from_static_str("localhost"),
            port: NonZeroU16::new(8080).expect("8080 is nonzero"),
            session_token: SessionToken::from(uuid::Uuid::nil()),
        }
    }

    #[test]
    fn cancellability_from_status_covers_every_status() {
        for (status, expected) in [
            (QueryJobStatus::Pending, Cancellability::Cancellable),
            (QueryJobStatus::Running, Cancellability::Cancellable),
            (
                QueryJobStatus::CancelRequested,
                Cancellability::CancellationRequested,
            ),
            (
                QueryJobStatus::Cancelling,
                Cancellability::CancellationRequested,
            ),
            (
                QueryJobStatus::Cancelled,
                Cancellability::CancellationRequested,
            ),
            (QueryJobStatus::Succeeded, Cancellability::NotCancellable),
            (QueryJobStatus::Failed, Cancellability::NotCancellable),
            (QueryJobStatus::Killed, Cancellability::NotCancellable),
        ] {
            assert_eq!(
                Cancellability::from_status(status),
                expected,
                "status: {status:?}"
            );
        }
    }

    #[test]
    fn validate_accepts_a_query_with_any_ordered_time_range() {
        for (begin_timestamp, end_timestamp) in [
            (None, None),
            (Some(1), None),
            (None, Some(1)),
            (Some(1), Some(1)),
        ] {
            let config = SearchJobConfig {
                query_string: "*".to_owned(),
                begin_timestamp,
                end_timestamp,
                ..SearchJobConfig::default()
            };
            assert_eq!(config.validate(), Ok(()));
        }
    }

    #[test]
    fn validate_accepts_max_num_results_or_a_network_output_alone() {
        for config in [
            SearchJobConfig {
                query_string: "*".to_owned(),
                max_num_results: NonZeroU32::new(1),
                ..SearchJobConfig::default()
            },
            SearchJobConfig {
                query_string: "*".to_owned(),
                network_output: Some(network_output()),
                ..SearchJobConfig::default()
            },
        ] {
            assert_eq!(config.validate(), Ok(()), "{config:?}");
        }
    }

    #[test]
    fn validate_rejects_unsupported_settings() {
        let valid = SearchJobConfig {
            query_string: "*".to_owned(),
            ..SearchJobConfig::default()
        };
        for (config, expected_error) in [
            (
                SearchJobConfig {
                    aggregation_config: Some(AggregationConfig::default()),
                    ..valid.clone()
                },
                SearchJobConfigError::AggregationUnsupported,
            ),
            (
                SearchJobConfig {
                    write_to_file: true,
                    ..valid.clone()
                },
                SearchJobConfigError::WriteToFileUnsupported,
            ),
            (
                SearchJobConfig {
                    network_output: Some(network_output()),
                    max_num_results: NonZeroU32::new(1),
                    ..valid.clone()
                },
                SearchJobConfigError::MaxNumResultsUnsupported,
            ),
            (
                SearchJobConfig {
                    query_string: String::new(),
                    ..valid.clone()
                },
                SearchJobConfigError::EmptyQuery,
            ),
            (
                SearchJobConfig {
                    begin_timestamp: Some(2),
                    end_timestamp: Some(1),
                    ..valid
                },
                SearchJobConfigError::ReversedTimeRange {
                    begin_timestamp: 2,
                    end_timestamp: 1,
                },
            ),
        ] {
            assert_eq!(config.validate(), Err(expected_error));
        }
    }
}
