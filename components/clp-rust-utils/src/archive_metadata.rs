use serde::{Deserialize, Serialize};
use sqlx::Row;

/// Table name for archive time-range metadata used for query-time pruning.
pub const ARCHIVE_METADATA_TABLE_NAME: &str = "archive_time_metadata";

/// Default number of concurrent S3 object fetches when streaming search results.
pub const DEFAULT_S3_FETCH_CONCURRENCY: usize = 16;

/// Represents the time-range metadata for a single archive stored in the metadata DB.
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
pub struct ArchiveTimeMetadata {
    pub archive_id: String,
    pub dataset: String,
    pub min_timestamp: i64,
    pub max_timestamp: i64,
    pub s3_key: String,
    pub size_bytes: u64,
}

/// Creates the `archive_time_metadata` table if it does not already exist.
///
/// # Errors
///
/// Forwards [`sqlx::query::Query::execute`]'s errors on failure.
pub async fn create_archive_metadata_table(
    pool: &sqlx::MySqlPool,
) -> Result<(), sqlx::Error> {
    sqlx::query(&format!(
        "CREATE TABLE IF NOT EXISTS `{ARCHIVE_METADATA_TABLE_NAME}` (
            `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
            `archive_id` VARCHAR(64) NOT NULL,
            `dataset` VARCHAR(255) NOT NULL DEFAULT 'default',
            `min_timestamp` BIGINT NOT NULL,
            `max_timestamp` BIGINT NOT NULL,
            `s3_key` VARCHAR(1024) NOT NULL DEFAULT '',
            `size_bytes` BIGINT UNSIGNED NOT NULL DEFAULT 0,
            PRIMARY KEY (`id`),
            UNIQUE KEY `uk_archive_id` (`archive_id`),
            INDEX `idx_dataset_time` (`dataset`, `min_timestamp`, `max_timestamp`)
        )"
    ))
    .execute(pool)
    .await?;
    Ok(())
}

/// Inserts or updates archive time-range metadata (upsert on `archive_id`).
///
/// # Errors
///
/// Forwards [`sqlx::query::Query::execute`]'s errors on failure.
pub async fn upsert_archive_metadata(
    pool: &sqlx::MySqlPool,
    metadata: &ArchiveTimeMetadata,
) -> Result<(), sqlx::Error> {
    sqlx::query(&format!(
        "INSERT INTO `{ARCHIVE_METADATA_TABLE_NAME}`
            (`archive_id`, `dataset`, `min_timestamp`, `max_timestamp`, `s3_key`, `size_bytes`)
         VALUES (?, ?, ?, ?, ?, ?)
         ON DUPLICATE KEY UPDATE
            `dataset` = VALUES(`dataset`),
            `min_timestamp` = VALUES(`min_timestamp`),
            `max_timestamp` = VALUES(`max_timestamp`),
            `s3_key` = VALUES(`s3_key`),
            `size_bytes` = VALUES(`size_bytes`)"
    ))
    .bind(&metadata.archive_id)
    .bind(&metadata.dataset)
    .bind(metadata.min_timestamp)
    .bind(metadata.max_timestamp)
    .bind(&metadata.s3_key)
    .bind(metadata.size_bytes)
    .execute(pool)
    .await?;
    Ok(())
}

/// Queries archive IDs whose time ranges overlap with the given search window.
///
/// Archives are pruned when their entire time range falls outside the query window.
/// If both `begin` and `end` are `None`, all archive IDs for the given datasets are returned.
///
/// # Errors
///
/// Forwards [`sqlx::query::Query::fetch_all`]'s errors on failure.
pub async fn query_overlapping_archives(
    pool: &sqlx::MySqlPool,
    datasets: &[String],
    begin_timestamp: Option<i64>,
    end_timestamp: Option<i64>,
) -> Result<Vec<String>, sqlx::Error> {
    if datasets.is_empty() {
        return Ok(Vec::new());
    }

    let placeholders: String = datasets.iter().map(|_| "?").collect::<Vec<_>>().join(", ");

    let (where_clause, has_begin, has_end) = match (begin_timestamp, end_timestamp) {
        (Some(_), Some(_)) => (
            format!(
                "WHERE `dataset` IN ({placeholders}) \
                 AND `max_timestamp` >= ? AND `min_timestamp` <= ?"
            ),
            true,
            true,
        ),
        (Some(_), None) => (
            format!(
                "WHERE `dataset` IN ({placeholders}) AND `max_timestamp` >= ?"
            ),
            true,
            false,
        ),
        (None, Some(_)) => (
            format!(
                "WHERE `dataset` IN ({placeholders}) AND `min_timestamp` <= ?"
            ),
            false,
            true,
        ),
        (None, None) => (
            format!("WHERE `dataset` IN ({placeholders})"),
            false,
            false,
        ),
    };

    let sql = format!(
        "SELECT `archive_id` FROM `{ARCHIVE_METADATA_TABLE_NAME}` {where_clause}"
    );

    let mut query = sqlx::query(&sql);
    for ds in datasets {
        query = query.bind(ds);
    }
    if has_begin {
        query = query.bind(begin_timestamp.unwrap());
    }
    if has_end {
        query = query.bind(end_timestamp.unwrap());
    }

    let rows = query.fetch_all(pool).await?;
    let ids = rows
        .iter()
        .map(|row| row.get::<String, _>("archive_id"))
        .collect();
    Ok(ids)
}
