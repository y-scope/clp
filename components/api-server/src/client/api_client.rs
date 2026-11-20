//! The client shared by the API server's routes.

use std::sync::Arc;

use clp_rust_utils::clp_config::package::config::Config;
use query_result_listener::listener::ResultListener;

use crate::sse::SseEncoding;

#[derive(Clone)]
pub struct Client {
    #[allow(clippy::struct_field_names)]
    pub(super) mongodb_client: mongodb::Client,
    pub(super) sql_pool: sqlx::Pool<sqlx::MySql>,
    pub(super) config: Config,
    pub(super) query_result_listener: Arc<ResultListener<SseEncoding>>,
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
