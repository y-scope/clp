//! A client for the API server's query and compression operations.

mod api_client;
mod compression;
mod query;

pub use api_client::Client;
pub use clp_rust_utils::job_config::CompressionJobStatus;
pub use compression::CompressionUsage;
pub use compression::CompressionUsageParams;
pub use compression::ValidatedCompressionUsageParams;
pub use query::QueryConfig;
pub use query::QueryOutputConfig;
pub use query::SearchResultStream;

pub use crate::error::ClientError;
