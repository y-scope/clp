//! A client for the public API's operations.

mod compression;
mod query;
mod search;
mod timeline;

use clp_rust_utils::clp_config::package::config::Config;
pub use clp_rust_utils::job_config::CompressionJobStatus;
pub use compression::CompressionUsage;
pub use compression::CompressionUsageParams;
pub use compression::DEFAULT_JOB_STATUSES;
pub use compression::ValidatedCompressionUsageParams;
pub use query::QueryConfig;
pub use query::QueryOutputConfig;
pub use query::StreamingQueryConfig;
pub use search::SearchResultStream;
pub use timeline::TimelineBucket;

pub use crate::error::ClientError;

#[derive(Clone)]
pub struct Client {
    #[allow(clippy::struct_field_names)]
    mongodb_client: mongodb::Client,
    sql_pool: sqlx::Pool<sqlx::MySql>,
    config: Config,
}

impl Client {
    /// Factory function.
    ///
    /// Creates a client that shares the given `MongoDB` client and `MySQL` connection pool.
    ///
    /// # Returns
    ///
    /// A newly created [`Client`].
    #[must_use]
    pub const fn new(
        config: Config,
        mongodb_client: mongodb::Client,
        sql_pool: sqlx::Pool<sqlx::MySql>,
    ) -> Self {
        Self {
            mongodb_client,
            sql_pool,
            config,
        }
    }

    /// # Returns
    ///
    /// A reference to the API server configuration.
    ///
    /// # Panics
    ///
    /// Panics if `self.config.api_server` is `None`.
    const fn get_api_server_config(
        &self,
    ) -> &clp_rust_utils::clp_config::package::config::ApiServer {
        self.config
            .api_server
            .as_ref()
            .expect("api_server configuration is missing")
    }
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroU32;
    use std::str::FromStr;

    use clp_rust_utils::job_config::Aggregation;
    use clp_rust_utils::job_config::QueryOutput;
    use clp_rust_utils::job_config::ResultsCacheOutput;

    use super::*;

    #[test]
    fn parse_job_status_pending_not_in_defaults() {
        assert_eq!(
            CompressionJobStatus::from_str("PENDING"),
            Ok(CompressionJobStatus::Pending)
        );
        assert!(!DEFAULT_JOB_STATUSES.contains(&CompressionJobStatus::Pending));
    }

    #[test]
    fn reject_duplicate_job_status() {
        let params = CompressionUsageParams {
            time_range_begin_millisecs: 0,
            time_range_end_millisecs: 1,
            job_status: Some("succeeded,succeeded".to_owned()),
            limit: 100,
        };
        let result = ValidatedCompressionUsageParams::try_from(params);
        assert!(result.is_err());
    }

    #[test]
    fn query_output_config_converts_to_the_matching_query_output() {
        use clp_rust_utils::types::PositiveI64;

        let max_num_results = NonZeroU32::new(7);
        let aggregation = Some(Aggregation::CountByTime {
            bucket_size_millisecs: PositiveI64::new(60_000).expect("60,000 is positive"),
        });
        for (output, expected) in [
            (
                QueryOutputConfig::ResultsCache {
                    max_num_results,
                    aggregation,
                },
                QueryOutput::ResultsCache(ResultsCacheOutput {
                    max_num_results,
                    aggregation,
                }),
            ),
            (QueryOutputConfig::File {}, QueryOutput::File {}),
        ] {
            assert_eq!(
                QueryOutput::from(output.clone()),
                expected,
                "output: {output:?}"
            );
        }
    }
}
