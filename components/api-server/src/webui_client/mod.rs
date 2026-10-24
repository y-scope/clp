//! A client for the operations that back the `WebUI`'s routes.

mod metadata;

use clp_rust_utils::clp_config::package::config::Config;
pub use metadata::*;

/// A client for the operations that back the `WebUI`'s routes.
#[derive(Clone)]
pub struct WebuiClient {
    sql_pool: sqlx::Pool<sqlx::MySql>,
    config: Config,
}

impl WebuiClient {
    /// Factory function.
    ///
    /// Creates a client that shares the given `MySQL` connection pool.
    ///
    /// # Returns
    ///
    /// A newly created [`WebuiClient`].
    #[must_use]
    pub const fn new(config: Config, sql_pool: sqlx::Pool<sqlx::MySql>) -> Self {
        Self { sql_pool, config }
    }
}
