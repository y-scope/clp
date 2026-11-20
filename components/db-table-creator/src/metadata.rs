//! The clp-s metadata tables, which every dataset shares.

use clp_rust_utils::clp_config::package::config::Database as DatabaseConfig;

/// Creates the clp-s metadata tables that don't exist yet.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
pub async fn create_tables(
    db_pool: &sqlx::MySqlPool,
    database_config: &DatabaseConfig,
) -> Result<(), sqlx::Error> {
    let datasets_table = database_config.datasets_table_name();
    let archives_table = database_config.unscoped_archives_table_name();
    let long_span_archives_table = database_config.long_span_archives_table_name();
    let column_metadata_table = database_config.unscoped_column_metadata_table_name();

    let statements = [
        format!(
            r"
            CREATE TABLE IF NOT EXISTS `{datasets_table}` (
                `id` SMALLINT unsigned NOT NULL AUTO_INCREMENT,
                `name` VARCHAR(255) NOT NULL,
                `archive_storage_path` VARCHAR(4096) NOT NULL,
                `retention_period_minutes` INT unsigned DEFAULT NULL,
                `is_deleted` BOOLEAN NOT NULL DEFAULT FALSE,
                UNIQUE KEY `dataset_name` (`name`) USING BTREE,
                PRIMARY KEY (`id`)
            )"
        ),
        // `archive_uuid` can't be a unique key, since MySQL requires every unique key of a
        // partitioned table to contain the partitioning column.
        format!(
            r"
            CREATE TABLE IF NOT EXISTS `{archives_table}` (
                `id` BIGINT unsigned NOT NULL AUTO_INCREMENT,
                `dataset_id` SMALLINT unsigned NOT NULL,
                `uuid` BINARY(16) NOT NULL,
                `timestamp_range_begin_millis` BIGINT NOT NULL,
                `timestamp_range_end_millis` BIGINT NOT NULL,
                `num_uncompressed_bytes` BIGINT unsigned NOT NULL,
                `num_compressed_bytes` BIGINT unsigned NOT NULL,
                `creation_time_millis` BIGINT NOT NULL,
                `pack_id` BIGINT unsigned DEFAULT NULL,
                `is_deleted` BOOLEAN NOT NULL DEFAULT FALSE,
                PRIMARY KEY (`dataset_id`, `timestamp_range_begin_millis`, `id`),
                KEY `auto_inc` (`id`),
                KEY `archive_uuid` (`dataset_id`, `uuid`),
                KEY `archives_gc_order` (`dataset_id`, `creation_time_millis`)
            ) ENGINE=InnoDB ROW_FORMAT=COMPRESSED KEY_BLOCK_SIZE=8
            PARTITION BY RANGE (`timestamp_range_begin_millis`) (
                PARTITION `p_future` VALUES LESS THAN MAXVALUE
            )"
        ),
        // This table holds a second reference to every archive whose timestamp range exceeds the
        // span contract, so it's deliberately unpartitioned.
        format!(
            r"
            CREATE TABLE IF NOT EXISTS `{long_span_archives_table}` (
                `dataset_id` SMALLINT unsigned NOT NULL,
                `timestamp_range_begin_millis` BIGINT NOT NULL,
                `timestamp_range_end_millis` BIGINT NOT NULL,
                `archive_id` BIGINT unsigned NOT NULL,
                PRIMARY KEY (`dataset_id`, `timestamp_range_end_millis`, `archive_id`),
                KEY `by_begin` (`dataset_id`, `timestamp_range_begin_millis`)
            ) ENGINE=InnoDB"
        ),
        format!(
            r"
            CREATE TABLE IF NOT EXISTS `{column_metadata_table}` (
                `dataset_id` SMALLINT unsigned NOT NULL,
                `name` VARCHAR(512) NOT NULL,
                `type` TINYINT NOT NULL,
                PRIMARY KEY (`dataset_id`, `name`, `type`)
            )"
        ),
    ];
    for statement in &statements {
        sqlx::query(statement).execute(db_pool).await?;
    }

    Ok(())
}
