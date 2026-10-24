//! A client for the operations that back the `WebUI`'s routes.

mod metadata;
mod stream_files;

use clp_rust_utils::clp_config::package::config::Config;
pub use metadata::*;
pub use stream_files::*;

/// A client for the operations that back the `WebUI`'s routes.
#[derive(Clone)]
pub struct WebuiClient {
    sql_pool: sqlx::Pool<sqlx::MySql>,
    mongodb_client: mongodb::Client,
    stream_file_extraction: StreamFileExtractionSettings,
    config: Config,
}

impl WebuiClient {
    /// Factory function.
    ///
    /// Creates a client that shares the given `MySQL` connection pool and `MongoDB` client, and
    /// that extracts stream files with the given settings.
    ///
    /// # Returns
    ///
    /// A newly created [`WebuiClient`].
    #[must_use]
    pub const fn new(
        config: Config,
        sql_pool: sqlx::Pool<sqlx::MySql>,
        mongodb_client: mongodb::Client,
        stream_file_extraction: StreamFileExtractionSettings,
    ) -> Self {
        Self {
            sql_pool,
            mongodb_client,
            stream_file_extraction,
            config,
        }
    }
}
