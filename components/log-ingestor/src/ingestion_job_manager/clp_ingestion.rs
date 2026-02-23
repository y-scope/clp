use async_trait::async_trait;
use clp_rust_utils::{
    database::mysql::MySqlEnumFormat,
    job_config::ingestion::s3::S3IngestionJobConfig,
    s3::ObjectMetadata,
};
use const_format::formatcp;
use sqlx::MySqlPool;
use strum_macros::{AsRefStr, Display, EnumIter, EnumString};

use crate::{
    ingestion_job::{IngestionJobState, S3ScannerState, SqsListenerState},
    ingestion_job_manager::IngestionJobId,
};

/// Connector for managing ingestion jobs in CLP DB.
pub struct ClpDbIngestionConnector {
    db_pool: MySqlPool,
}

impl ClpDbIngestionConnector {
    /// Factory function.
    ///
    /// This method creates a new CLP DB connector and creates all necessary tables if they do not
    /// already exist.
    ///
    /// # Returns
    ///
    /// A newly created instance with the given database pool.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure if:
    ///   * [`INGESTION_JOB_TABLE_NAME`] table creation query execution fails.
    ///   * [`INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME`] table creation query execution fails.
    ///   * [`INGESTED_S3_OBJECT_METADATA_TABLE_NAME`] table creation query execution fails.
    pub async fn new(db_pool: MySqlPool) -> anyhow::Result<Self> {
        sqlx::query(ingestion_job_table_creation_query().as_str())
            .execute(&db_pool)
            .await?;

        sqlx::query(ingestion_job_s3_scanner_state_table_creation_query().as_str())
            .execute(&db_pool)
            .await?;

        sqlx::query(ingested_s3_object_metadata_table_creation_query().as_str())
            .execute(&db_pool)
            .await?;

        Ok(Self { db_pool })
    }

    /// Creates a new ingestion job in the CLP database with the given configuration and returns its
    /// initial state.
    ///
    /// # Returns
    ///
    /// A [`ClpIngestionState`] instance representing the initial state of the newly created
    /// ingestion job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    /// * Forwards [`serde_json::to_string`]'s return values on failure.
    pub async fn create_ingestion_job(
        &self,
        config: S3IngestionJobConfig,
    ) -> anyhow::Result<ClpIngestionState> {
        const QUERY: &str = formatcp!(
            r"INSERT INTO `{table}` (`config`) VALUES (?);",
            table = INGESTION_JOB_TABLE_NAME,
        );

        let mut tx = self.db_pool.begin().await?;
        let job_id = sqlx::query(QUERY)
            .bind(serde_json::to_string(&config)?)
            .execute(&mut *tx)
            .await?
            .last_insert_id();

        if let S3IngestionJobConfig::S3Scanner(_) = config {
            const S3_SCANNER_STATE_INSERT_QUERY: &str = formatcp!(
                r"INSERT INTO `{table}` (`id`) VALUES (?);",
                table = INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME,
            );
            sqlx::query(S3_SCANNER_STATE_INSERT_QUERY)
                .bind(job_id)
                .execute(&mut *tx)
                .await?;
        }

        tx.commit().await?;

        Ok(ClpIngestionState {
            job_id,
            db_pool: self.db_pool.clone(),
        })
    }
}

/// A CLP-DB-backed implementation of ingestion job state. This state ingests object metadata into
/// the CLP system and persists all ingestion progress and state transitions in the CLP database.
#[derive(Clone)]
pub struct ClpIngestionState {
    job_id: IngestionJobId,
    db_pool: MySqlPool,
}

impl ClpIngestionState {
    /// Updates the status of the underlying ingestion job in the CLP database.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if no rows were affected by the job status update, which indicates the
    ///   job may not exist.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn update_job_status(&self, status: ClpIngestionJobStatus) -> anyhow::Result<()> {
        const QUERY: &str = formatcp!(
            r"UPDATE `{table}` SET `status` = ? WHERE `id` = ?;",
            table = INGESTION_JOB_TABLE_NAME,
        );

        let result = sqlx::query(QUERY)
            .bind(status)
            .bind(self.job_id)
            .execute(&self.db_pool)
            .await?;

        if result.rows_affected() == 0 {
            return Err(anyhow::anyhow!(
                "Job status update failed. The job may not exist."
            ));
        }

        Ok(())
    }

    /// Ingests the given S3 object metadata into CLP DB.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// This method panics if:
    ///
    /// * `objects` is empty, as it cannot build a valid ingestion query in that case.
    async fn ingest_s3_object_metadata(
        &self,
        tx: &mut sqlx::Transaction<'_, sqlx::MySql>,
        objects: &[ObjectMetadata],
    ) -> anyhow::Result<()> {
        const BASE_INGESTION_QUERY: &str = formatcp!(
            r"INSERT INTO `{table}` (`bucket`, `key`, `size`, `ingestion_job_id`) VALUES ",
            table = INGESTED_S3_OBJECT_METADATA_TABLE_NAME,
        );

        assert!(
            !objects.is_empty(),
            "Cannot build S3 object metadata ingestion query with empty objects"
        );

        // Ingest object metadata
        // NOTE: MySQL has a maximum placeholder limit of 65535. We need to batch the ingestion to
        // avoid hitting this limit. If the number of placeholders per insert changes, we may need
        // to adjust the chunk size accordingly.
        for chunk in objects.chunks(10000) {
            let query_string = format!(
                "{}{}",
                BASE_INGESTION_QUERY,
                std::iter::repeat_n("(?, ?, ?, ?)", chunk.len())
                    .collect::<Vec<_>>()
                    .join(", ")
            );

            let mut query = sqlx::query(&query_string);
            for object in chunk {
                query = query
                    .bind(object.bucket.as_str())
                    .bind(object.key.as_str())
                    .bind(object.size)
                    .bind(self.job_id);
            }

            query.execute(&mut **tx).await?;
        }

        // Update ingestion job. This must be the last step in the transaction.
        Ok(())
    }

    /// Updates the ingestion stats of the underling job, and commits the transaction.
    ///
    /// # NOTE
    ///
    /// This method returns success only if the underlying job is valid and in
    /// [`ClpIngestionJobStatus::Running`] state. On failure, the transaction will be rolled back
    /// and all previous operations in the transaction will be dropped.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success, which indicates the ingestion stats have been updated and the
    /// transaction has been committed.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if no rows were affected by the job status update, which may indicates
    ///   the job doesn't exist, or it is not in the running state.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// This method panics if:
    ///
    /// * The number of objects exceeds `u64::MAX`, which should be practically impossible.
    async fn update_ingestion_stats_and_commit(
        &self,
        mut tx: sqlx::Transaction<'_, sqlx::MySql>,
        num_files_ingested: usize,
    ) -> anyhow::Result<()> {
        const BASE_JOB_UPDATE_QUERY: &str = formatcp!(
            r"UPDATE `{table}` SET `num_files_ingested` = `num_files_ingested` + ?
                WHERE `id` = ? AND `status` = ?;",
            table = INGESTION_JOB_TABLE_NAME,
        );

        let result = sqlx::query(BASE_JOB_UPDATE_QUERY)
            .bind(
                u64::try_from(num_files_ingested)
                    .expect("number of objects should not exceed u64::MAX"),
            )
            .bind(self.job_id)
            .bind(ClpIngestionJobStatus::Running)
            .execute(&mut *tx)
            .await?;

        if result.rows_affected() == 0 {
            return Err(anyhow::anyhow!(
                "Job status update failed. The job may not exist or is not in the running state."
            ));
        }

        tx.commit().await?;
        Ok(())
    }
}

#[async_trait]
impl IngestionJobState for ClpIngestionState {
    /// # Errors
    ///
    /// * Forwards [`Self::update_job_status`]'s return values on failure.
    async fn start(&self) -> anyhow::Result<()> {
        self.update_job_status(ClpIngestionJobStatus::Running).await
    }

    /// # Errors
    ///
    /// * Forwards [`Self::update_job_status`]'s return values on failure.
    async fn end(&self) -> anyhow::Result<()> {
        self.update_job_status(ClpIngestionJobStatus::Finished)
            .await
    }
}

#[async_trait]
impl S3ScannerState for ClpIngestionState {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure when updating the last
    ///   ingested key for the S3 scanner state.
    /// * Forwards [`Self::ingest_s3_object_metadata`]'s return values on failure.
    /// * Forwards [`Self::update_ingestion_stats_and_commit`]'s return values on failure.
    async fn ingest(
        &self,
        objects: Vec<ObjectMetadata>,
        last_ingested_key: &str,
    ) -> anyhow::Result<()> {
        const UPDATE_S3_SCANNER_STATE_QUERY: &str = formatcp!(
            r"UPDATE `{table}` SET `last_ingested_key` = ? WHERE `id` = ?;",
            table = INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME,
        );

        let mut tx = self.db_pool.begin().await?;

        if sqlx::query(UPDATE_S3_SCANNER_STATE_QUERY)
            .bind(last_ingested_key)
            .bind(self.job_id)
            .execute(&mut *tx)
            .await?
            .rows_affected()
            == 0
        {
            const ERROR_MSG: &str = "Failed to update last ingested key for S3 scanner state.";
            tracing::error!(
                job_id = ? self.job_id,
                last_ingested_key = ? last_ingested_key,
                ERROR_MSG
            );
            return Err(anyhow::anyhow!(ERROR_MSG));
        }

        self.ingest_s3_object_metadata(&mut tx, &objects)
            .await
            .inspect_err(|err| {
                tracing::error!(
                    error = ? err,
                    job_id = ? self.job_id,
                    "Failed to ingest S3 object metadata."
                );
            })?;

        self.update_ingestion_stats_and_commit(tx, objects.len())
            .await
            .inspect_err(|err| {
                tracing::error!(
                    error = ? err,
                    job_id = ? self.job_id,
                    "Failed to commit ingestion."
                );
            })
    }
}

#[async_trait]
impl SqsListenerState for ClpIngestionState {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`Self::ingest_s3_object_metadata`]'s return values on failure.
    /// * Forwards [`Self::update_ingestion_stats_and_commit`]'s return values on failure.
    async fn ingest(&self, objects: Vec<ObjectMetadata>) -> anyhow::Result<()> {
        let mut tx = self.db_pool.begin().await?;
        self.ingest_s3_object_metadata(&mut tx, &objects)
            .await
            .inspect_err(|err| {
                tracing::error!(
                    error = ? err,
                    job_id = ? self.job_id,
                    "Failed to ingest S3 object metadata."
                );
            })?;

        self.update_ingestion_stats_and_commit(tx, objects.len())
            .await
            .inspect_err(|err| {
                tracing::error!(
                    error = ? err,
                    job_id = ? self.job_id,
                    "Failed to commit ingestion."
                );
            })
    }
}

/// Enum for CLP ingestion job status.
#[derive(
    Debug,
    Clone,
    Copy,
    PartialEq,
    Eq,
    sqlx::Encode,
    sqlx::Decode,
    EnumIter,
    AsRefStr,
    Display,
    EnumString,
)]
#[sqlx(rename_all = "snake_case")]
#[strum(serialize_all = "snake_case")]
pub enum ClpIngestionJobStatus {
    Requested,
    Running,
    Paused,
    Failed,
    Finished,
}

impl MySqlEnumFormat for ClpIngestionJobStatus {}

impl sqlx::Type<sqlx::MySql> for ClpIngestionJobStatus {
    fn type_info() -> <sqlx::MySql as sqlx::Database>::TypeInfo {
        <str as sqlx::Type<sqlx::MySql>>::type_info()
    }

    fn compatible(ty: &<sqlx::MySql as sqlx::Database>::TypeInfo) -> bool {
        <str as sqlx::Type<sqlx::MySql>>::compatible(ty)
    }
}

/// Enum for CLP ingestion S3 object metadata status.
#[derive(
    Debug,
    Clone,
    Copy,
    PartialEq,
    Eq,
    sqlx::Encode,
    sqlx::Decode,
    EnumIter,
    AsRefStr,
    Display,
    EnumString,
)]
#[sqlx(rename_all = "snake_case")]
#[strum(serialize_all = "snake_case")]
pub enum IngestedS3ObjectMetadataStatus {
    Buffered,
    Submitted,
    Compressed,
    Failed,
}

impl MySqlEnumFormat for IngestedS3ObjectMetadataStatus {}

const INGESTION_JOB_TABLE_NAME: &str = "ingestion_job";
const INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME: &str = "ingestion_job_s3_scanner_state";
const INGESTED_S3_OBJECT_METADATA_TABLE_NAME: &str = "ingested_s3_object_metadata";

/// The query to create the table for CLP ingestion jobs.
#[must_use]
fn ingestion_job_table_creation_query() -> String {
    format!(
        r"
CREATE TABLE IF NOT EXISTS `{table}` (
    `id` BIGINT unsigned NOT NULL AUTO_INCREMENT,
    `config` TEXT NOT NULL,
    `status` {status_enum} NOT NULL DEFAULT '{default_status}',
    `num_files_ingested` BIGINT unsigned NOT NULL DEFAULT '0',
    `num_files_compressed` BIGINT unsigned NOT NULL DEFAULT '0',
    `error_msg` TEXT NULL DEFAULT NULL,
    `creation_ts` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `last_update_ts` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
        ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (`id`)
);",
        table = INGESTION_JOB_TABLE_NAME,
        status_enum = ClpIngestionJobStatus::format_as_sql_enum(),
        default_status = ClpIngestionJobStatus::Requested,
    )
}

/// The query to create the table for S3 scanner job state tracking.
#[must_use]
fn ingestion_job_s3_scanner_state_table_creation_query() -> String {
    format!(
        r"
CREATE TABLE IF NOT EXISTS `{INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME}` (
    `id` BIGINT unsigned NOT NULL,
    `last_ingested_key` VARCHAR(1024) NULL DEFAULT NULL,
    PRIMARY KEY (`id`),
    CONSTRAINT `{INGESTION_JOB_S3_SCANNER_STATE_TABLE_NAME}_ingestion_job_id_ref`
        FOREIGN KEY (`id`) REFERENCES `{INGESTION_JOB_TABLE_NAME}` (`id`)
        ON DELETE RESTRICT ON UPDATE RESTRICT
);",
    )
}

/// The query to create the table for ingested S3 object metadata.
#[must_use]
fn ingested_s3_object_metadata_table_creation_query() -> String {
    format!(
        r"
CREATE TABLE IF NOT EXISTS `{table}` (
    `id` BIGINT unsigned NOT NULL AUTO_INCREMENT,
    `bucket` VARCHAR(1024) NOT NULL,
    `key` VARCHAR(1024) NOT NULL,
    `size` BIGINT unsigned NOT NULL,
    `status` {status_enum} NOT NULL DEFAULT '{default_status}',
    `ingestion_job_id` BIGINT unsigned NOT NULL,
    `compression_job_id` INT NULL DEFAULT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_ingestion_job_id` (`ingestion_job_id`),
    KEY `idx_compression_job_id` (`compression_job_id`),
    CONSTRAINT `{table}_ingestion_job_id_ref`
        FOREIGN KEY (`ingestion_job_id`) REFERENCES `{job_table}` (`id`)
        ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT `{table}_compression_job_id_ref`
        FOREIGN KEY (`compression_job_id`) REFERENCES `{compression_job_table}` (`id`)
        ON DELETE RESTRICT ON UPDATE RESTRICT
);",
        table = INGESTED_S3_OBJECT_METADATA_TABLE_NAME,
        job_table = INGESTION_JOB_TABLE_NAME,
        status_enum = IngestedS3ObjectMetadataStatus::format_as_sql_enum(),
        default_status = IngestedS3ObjectMetadataStatus::Buffered,
        compression_job_table = crate::compression::CLP_COMPRESSION_JOB_TABLE_NAME,
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_clp_ingestion_job_status_enum() {
        assert_eq!(
            ClpIngestionJobStatus::format_as_sql_enum(),
            "ENUM('requested', 'running', 'paused', 'failed', 'finished')"
        );
    }

    #[test]
    fn test_ingested_s3_object_metadata_status_enum() {
        assert_eq!(
            IngestedS3ObjectMetadataStatus::format_as_sql_enum(),
            "ENUM('buffered', 'submitted', 'compressed', 'failed')"
        );
    }
}
