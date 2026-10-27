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
    stream_file_storage: StreamFileStorage,
    config: Config,
}

impl WebuiClient {
    /// Factory function.
    ///
    /// Creates a client that shares the given `MySQL` connection pool and `MongoDB` client. For S3
    /// stream output, also creates the S3 client that pre-signs the stream files' URLs.
    ///
    /// # Returns
    ///
    /// A newly created [`WebuiClient`].
    pub async fn new(
        config: Config,
        sql_pool: sqlx::Pool<sqlx::MySql>,
        mongodb_client: mongodb::Client,
    ) -> Self {
        let stream_file_storage =
            StreamFileStorage::from_config(&config.stream_output.storage).await;
        Self {
            sql_pool,
            mongodb_client,
            stream_file_storage,
            config,
        }
    }
}
