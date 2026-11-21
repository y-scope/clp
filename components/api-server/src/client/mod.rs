//! A client for the API server's query and compression operations.

mod compression;
mod query;

use std::sync::Arc;

use clp_rust_utils::clp_config::package::config::Config;
pub use clp_rust_utils::job_config::CompressionJobStatus;
pub use compression::CompressionUsage;
pub use compression::CompressionUsageParams;
pub use compression::ValidatedCompressionUsageParams;
pub use query::QueryConfig;
pub use query::QueryOutputConfig;
pub use query::SearchResultStream;
use query_result_listener::listener::ResultListener;

pub use crate::error::ClientError;
use crate::sse::SseEncoding;

#[derive(Clone)]
pub struct Client {
    #[allow(clippy::struct_field_names)]
    mongodb_client: mongodb::Client,
    sql_pool: sqlx::Pool<sqlx::MySql>,
    config: Config,
    query_result_listener: Arc<ResultListener<SseEncoding>>,
}

impl Client {
    /// Factory function.
    ///
    /// Creates a client that shares the given `MongoDB` client, `MySQL` connection pool and query
    /// result listener.
    ///
    /// # Returns
    ///
    /// A newly created [`Client`].
    #[must_use]
    pub const fn new(
        config: Config,
        mongodb_client: mongodb::Client,
        sql_pool: sqlx::Pool<sqlx::MySql>,
        query_result_listener: Arc<ResultListener<SseEncoding>>,
    ) -> Self {
        Self {
            mongodb_client,
            sql_pool,
            config,
            query_result_listener,
        }
    }
}
