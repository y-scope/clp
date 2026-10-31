//! The crate-level error type for the query coordinator.

use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::SearchJobConfigError;
use clp_rust_utils::task_io::query::QueryOptionError;
use spider_core::types::resource_group::ExternalResourceGroupCredentialsError;

/// Errors returned by the query coordinator.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("invalid coordinator configuration: {0}")]
    InvalidConfiguration(String),

    #[error("invalid Spider endpoint: {0}")]
    InvalidEndpoint(String),

    #[error("invalid query job configuration: {0}")]
    InvalidQueryJobConfig(String),

    #[error("invalid query options: {0}")]
    QueryOption(#[from] QueryOptionError),

    #[error("invalid query job status transition from {from:?} to {to:?}")]
    InvalidQueryJobStatusTransition {
        from: QueryJobStatus,
        to: QueryJobStatus,
    },

    #[error("the query job has already been cancelled")]
    QueryJobCancelled,

    #[error("metadata corrupted for query job {0}: its row no longer exists")]
    QueryJobMetadataCorrupted(QueryJobId),

    #[error("failed to load the resource group credentials: {0}")]
    ResourceGroupCredentials(#[from] ExternalResourceGroupCredentialsError),

    #[error("semaphore error: {0}")]
    Semaphore(String),

    #[error("spider request failure: {0}")]
    SpiderClient(#[from] spider_client::error::ClientError),

    #[error("sqlx error: {0}")]
    Sqlx(#[from] sqlx::Error),

    #[error("failed to build the query task graph: {0}")]
    TaskGraph(#[from] spider_core::task::Error),

    #[error("failed to serialize a task input: {0}")]
    TaskInputSerialization(#[from] spider_core::types::io::TaskGraphInputError),

    #[error("number of query tasks {0} exceeds `i32::MAX`")]
    TooManyQueryTasks(usize),

    #[error("unsupported query job configuration: {0}")]
    UnsupportedJobConfig(String),
}

impl From<SearchJobConfigError> for Error {
    fn from(error: SearchJobConfigError) -> Self {
        match error {
            SearchJobConfigError::AggregationUnsupported
            | SearchJobConfigError::WriteToFileUnsupported
            | SearchJobConfigError::MaxNumResultsUnsupported => {
                Self::UnsupportedJobConfig(error.to_string())
            }
            SearchJobConfigError::EmptyQuery => Self::QueryOption(QueryOptionError::EmptyQuery),
            SearchJobConfigError::ReversedTimeRange { .. } => {
                Self::InvalidQueryJobConfig(error.to_string())
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::job_config::SearchJobConfigError;
    use clp_rust_utils::task_io::query::QueryOptionError;

    use super::Error;

    #[test]
    fn search_job_config_errors_map_to_their_coordinator_errors() {
        let error = Error::from(SearchJobConfigError::AggregationUnsupported);
        assert!(
            matches!(error, Error::UnsupportedJobConfig(_)),
            "`AggregationUnsupported` mapped to {error:?}"
        );

        let error = Error::from(SearchJobConfigError::WriteToFileUnsupported);
        assert!(
            matches!(error, Error::UnsupportedJobConfig(_)),
            "`WriteToFileUnsupported` mapped to {error:?}"
        );

        let error = Error::from(SearchJobConfigError::MaxNumResultsUnsupported);
        assert!(
            matches!(error, Error::UnsupportedJobConfig(_)),
            "`MaxNumResultsUnsupported` mapped to {error:?}"
        );

        let error = Error::from(SearchJobConfigError::EmptyQuery);
        assert!(
            matches!(error, Error::QueryOption(QueryOptionError::EmptyQuery)),
            "`EmptyQuery` mapped to {error:?}"
        );

        let error = Error::from(SearchJobConfigError::ReversedTimeRange {
            begin_timestamp: 2,
            end_timestamp: 1,
        });
        assert!(
            matches!(error, Error::InvalidQueryJobConfig(_)),
            "`ReversedTimeRange` mapped to {error:?}"
        );
    }
}
