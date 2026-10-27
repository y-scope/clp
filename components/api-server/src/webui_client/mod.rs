//! A client for the operations that back the `WebUI`'s routes.

mod compression;
mod logs_input;
mod metadata;
mod stream_files;

use clp_rust_utils::clp_config::package::config::Config;
pub use compression::*;
pub use logs_input::*;
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

#[cfg(test)]
impl WebuiClient {
    /// Factory function.
    ///
    /// Creates a client whose `MySQL` and `MongoDB` connections are only established on first use.
    ///
    /// # Returns
    ///
    /// A newly created [`WebuiClient`].
    ///
    /// # Panics
    ///
    /// Panics if the clients can't be created.
    async fn new_lazy(config: Config) -> Self {
        let sql_pool = sqlx::mysql::MySqlPoolOptions::new()
            .connect_lazy("mysql://127.0.0.1:1/clp-db")
            .expect("the MySQL URI should be valid");
        let mongodb_client =
            mongodb::Client::with_uri_str("mongodb://127.0.0.1:1/?serverSelectionTimeoutMS=100")
                .await
                .expect("the MongoDB URI should be valid");
        Self::new(config, sql_pool, mongodb_client).await
    }
}
