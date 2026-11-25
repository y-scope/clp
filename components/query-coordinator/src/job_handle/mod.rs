//! Handles for driving query jobs to completion.

mod archive;
mod query;

use std::num::NonZeroU64;
use std::num::NonZeroUsize;
use std::time::Duration;

use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::types::ArchiveUuid;
use sqlx::MySqlPool;

pub use self::query::QueryJobHandle;

/// Metadata for an archive in a dataset.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ArchiveMetadata {
    /// The archive's UUID.
    pub uuid: ArchiveUuid,

    /// The archive's dataset.
    pub dataset: DatasetName,

    /// The archive's uncompressed size in bytes.
    pub uncompressed_size: u64,

    /// The archive's end timestamp in Unix epoch milliseconds.
    pub end_timestamp: i64,
}

/// Options for a query job running in Spider.
pub struct SpiderOption {
    pub poll_interval: Duration,
    pub query_task_max_retry: u32,
}

/// Options for selecting the archives to search.
pub struct ArchiveSelectionOptions {
    pub archive_retention_period_millisecs: Option<NonZeroU64>,
    pub max_datasets_per_query: Option<NonZeroUsize>,
}

/// Resources shared by query job handles created by the coordinator.
pub struct QueryJobHandleContext {
    pub db_pool: MySqlPool,
    pub db_config: Database,
    pub archive_selection_options: ArchiveSelectionOptions,
    pub spider_option: SpiderOption,
}
