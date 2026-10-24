//! Read-only metadata about the CLP package's datasets, archives, and jobs.

use chrono::DateTime;
use chrono::Utc;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::COMPRESSION_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QUERY_TASKS_TABLE_NAME;
use clp_rust_utils::serde::ZstdMsgpack;
use serde::Deserialize;
use serde::Serialize;
use sqlx::Row;
use utoipa::ToSchema;

use super::WebuiClient;
use crate::error::ClientError;

/// Fields from a stored compression job config that the `WebUI` may display.
#[derive(Clone, Debug, Deserialize, ToSchema)]
pub struct CompressionMetadataDisplayConfig {
    pub input: CompressionMetadataDisplayInput,
    pub output: CompressionMetadataDisplayOutput,
}

/// Partial input fields kept for older compression jobs.
#[derive(Clone, Debug, Deserialize, ToSchema)]
pub struct CompressionMetadataDisplayInput {
    pub r#type: Option<String>,
    pub dataset: Option<String>,
    pub paths_to_compress: Option<Vec<String>>,
    pub path_prefix_to_remove: Option<String>,
    pub keys: Option<Vec<String>>,
    pub timestamp_key: Option<String>,
    pub unstructured: Option<bool>,
}

/// Partial output fields kept for older compression jobs.
#[derive(Clone, Debug, Deserialize, ToSchema)]
pub struct CompressionMetadataDisplayOutput {
    pub compression_level: Option<f64>,
    pub target_archive_size: Option<f64>,
    pub target_dictionaries_size: Option<f64>,
    pub target_encoded_file_size: Option<f64>,
    pub target_segment_size: Option<f64>,
}

/// A row of compression metadata, with the decoded CLP IO config.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct CompressionMetadata {
    /// The compression job's ID. Named `_id` to match the webui's existing JSON contract.
    #[serde(rename = "_id")]
    pub id: i64,
    /// Current status of the job. Matches `CompressionJobStatus` in
    /// `job_orchestration.scheduler.constants`.
    pub status: i32,
    /// Status message for the job.
    pub status_msg: String,
    /// Time the job started executing (RFC 3339). Absent if the job hasn't started.
    pub start_time: Option<String>,
    /// Time the job was last updated (RFC 3339).
    pub update_time: String,
    /// Wall-clock duration the job ran, in seconds. Absent if the job did not complete.
    pub duration: Option<f64>,
    /// Total uncompressed size of input files, in bytes.
    pub uncompressed_size: i64,
    /// Total compressed archive size, in bytes.
    pub compressed_size: i64,
    /// Display fields from the decoded CLP IO config. Storage credentials are omitted.
    #[schema(value_type = CompressionMetadataDisplayConfig)]
    pub clp_config: serde_json::Value,
}

impl TryFrom<sqlx::mysql::MySqlRow> for CompressionMetadata {
    type Error = ClientError;

    fn try_from(row: sqlx::mysql::MySqlRow) -> Result<Self, Self::Error> {
        let clp_config: &[u8] = row.try_get("clp_config")?;
        let clp_config = compression_metadata_config(ZstdMsgpack::deserialize(clp_config)?);
        Ok(Self {
            id: row.try_get("id")?,
            status: row.try_get("status")?,
            status_msg: row.try_get("status_msg")?,
            start_time: row
                .try_get::<Option<DateTime<Utc>>, _>("start_time")
                .ok()
                .flatten()
                .map(|time| time.to_rfc3339()),
            update_time: row
                .try_get::<Option<DateTime<Utc>>, _>("update_time")
                .ok()
                .flatten()
                .map_or_else(String::new, |time| time.to_rfc3339()),
            duration: row.try_get("duration")?,
            uncompressed_size: row.try_get("uncompressed_size")?,
            compressed_size: row.try_get("compressed_size")?,
            clp_config,
        })
    }
}

/// Aggregated space-savings statistics.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct SpaceSavings {
    /// Total uncompressed size of all archives, in bytes.
    pub total_uncompressed_size: i64,
    /// Total compressed size of all archives, in bytes.
    pub total_compressed_size: i64,
}

impl TryFrom<sqlx::mysql::MySqlRow> for SpaceSavings {
    type Error = sqlx::Error;

    fn try_from(row: sqlx::mysql::MySqlRow) -> Result<Self, Self::Error> {
        Ok(Self {
            total_uncompressed_size: row.try_get("total_uncompressed_size")?,
            total_compressed_size: row.try_get("total_compressed_size")?,
        })
    }
}

/// Ingestion details statistics. The whole value is `None` when no data has been ingested
/// yet.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct IngestionDetails {
    /// Earliest log entry timestamp (epoch milliseconds).
    pub begin_timestamp: i64,
    /// Latest log entry timestamp (epoch milliseconds).
    pub end_timestamp: i64,
    /// Number of distinct ingested files.
    pub num_files: i64,
    /// Total number of ingested messages.
    pub num_messages: i64,
}

impl TryFrom<sqlx::mysql::MySqlRow> for IngestionDetails {
    type Error = sqlx::Error;

    fn try_from(row: sqlx::mysql::MySqlRow) -> Result<Self, Self::Error> {
        Ok(Self {
            begin_timestamp: row.try_get("begin_timestamp")?,
            end_timestamp: row.try_get("end_timestamp")?,
            num_files: row.try_get("num_files")?,
            num_messages: row.try_get("num_messages")?,
        })
    }
}

/// Query-speed statistics for a search job. The whole value is `None` until the job has
/// scanned archives and finished (its duration is recorded only on completion).
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct QuerySpeed {
    /// Total uncompressed size of the archives the job scanned, in bytes.
    pub bytes: f64,
    /// Wall-clock duration the job ran, in seconds.
    pub duration: f64,
}

impl TryFrom<sqlx::mysql::MySqlRow> for QuerySpeed {
    type Error = sqlx::Error;

    fn try_from(row: sqlx::mysql::MySqlRow) -> Result<Self, Self::Error> {
        Ok(Self {
            bytes: row.try_get("bytes")?,
            duration: row.try_get("duration")?,
        })
    }
}

/// Earliest and latest log entry timestamps across the selected datasets. The whole value
/// is `None` when no archives exist yet (or, for CLP-S, when no datasets were selected).
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct TimeRange {
    /// Earliest log entry timestamp (epoch milliseconds).
    pub begin_timestamp: i64,
    /// Latest log entry timestamp (epoch milliseconds).
    pub end_timestamp: i64,
}

impl TryFrom<sqlx::mysql::MySqlRow> for TimeRange {
    type Error = sqlx::Error;

    fn try_from(row: sqlx::mysql::MySqlRow) -> Result<Self, Self::Error> {
        Ok(Self {
            begin_timestamp: row.try_get("begin_timestamp")?,
            end_timestamp: row.try_get("end_timestamp")?,
        })
    }
}

impl WebuiClient {
    /// Fetches all dataset names from the datasets table.
    ///
    /// # Returns
    ///
    /// The dataset names, ordered by name, on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryScalar::fetch_all`]'s return values on failure.
    pub async fn get_dataset_names(&self) -> Result<Vec<DatasetName>, ClientError> {
        let table = self.config.database.datasets_table_name();
        let names = sqlx::query_scalar(&format!("SELECT name FROM `{table}` ORDER BY name"))
            .fetch_all(&self.sql_pool)
            .await?;
        Ok(names)
    }

    /// Fetches the earliest and latest log entry timestamps across the given datasets.
    ///
    /// For the CLP storage engine, `datasets` is ignored and the single archives table is queried.
    /// For CLP-S, the union of the per-dataset archives tables is queried.
    ///
    /// # Returns
    ///
    /// The earliest and latest log entry timestamps on success, or `None` when no archives exist
    /// yet (or, for CLP-S, when `datasets` is empty).
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
    pub async fn get_time_range(
        &self,
        datasets: &[DatasetName],
    ) -> Result<Option<TimeRange>, ClientError> {
        let database = &self.config.database;
        let sql = match self.config.package.storage_engine {
            StorageEngine::Clp => format!(
                "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS \
                 end_timestamp FROM `{}`",
                database.unscoped_archives_table_name()
            ),
            StorageEngine::ClpS => {
                if datasets.is_empty() {
                    return Ok(None);
                }
                let union = union_all(datasets.iter().map(|dataset| {
                    format!(
                        "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS \
                         end_timestamp FROM `{}`",
                        database.archives_table_name(dataset)
                    )
                }));
                format!(
                    "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS \
                     end_timestamp FROM ({union}) AS combined"
                )
            }
        };
        let row = sqlx::query(&sql).fetch_one(&self.sql_pool).await?;
        Ok(TimeRange::try_from(row).ok())
    }

    /// Fetches aggregated space-savings statistics across all datasets.
    ///
    /// # Returns
    ///
    /// The total uncompressed and compressed sizes on success; both are `0` when no data has been
    /// ingested yet.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::get_dataset_names`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
    /// * Forwards [`SpaceSavings::try_from`]'s return values on failure.
    pub async fn get_space_savings(&self) -> Result<SpaceSavings, ClientError> {
        let database = &self.config.database;
        let sql = match self.config.package.storage_engine {
            StorageEngine::Clp => format!(
                "SELECT CAST(COALESCE(SUM(uncompressed_size), 0) AS SIGNED) AS \
                 total_uncompressed_size, CAST(COALESCE(SUM(size), 0) AS SIGNED) AS \
                 total_compressed_size FROM `{}`",
                database.unscoped_archives_table_name()
            ),
            StorageEngine::ClpS => {
                let datasets = self.get_dataset_names().await?;
                if datasets.is_empty() {
                    return Ok(SpaceSavings {
                        total_uncompressed_size: 0,
                        total_compressed_size: 0,
                    });
                }
                let union = union_all(datasets.iter().map(|dataset| {
                    format!(
                        "SELECT SUM(uncompressed_size) AS uncompressed_size, SUM(size) AS size \
                         FROM `{}`",
                        database.archives_table_name(dataset)
                    )
                }));
                format!(
                    "SELECT CAST(COALESCE(SUM(uncompressed_size), 0) AS SIGNED) AS \
                     total_uncompressed_size, CAST(COALESCE(SUM(size), 0) AS SIGNED) AS \
                     total_compressed_size FROM ({union}) AS archives_combined"
                )
            }
        };
        let row = sqlx::query(&sql).fetch_one(&self.sql_pool).await?;
        Ok(SpaceSavings::try_from(row)?)
    }

    /// Fetches ingestion details (timestamp range, file count, message count) across all
    /// datasets.
    ///
    /// # Returns
    ///
    /// The ingestion details on success, or `None` when no data has been ingested yet.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::get_dataset_names`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
    pub async fn get_ingestion_details(&self) -> Result<Option<IngestionDetails>, ClientError> {
        let database = &self.config.database;
        let sql = match self.config.package.storage_engine {
            StorageEngine::Clp => {
                let archives = database.unscoped_archives_table_name();
                let files = database.unscoped_files_table_name();
                format!(
                    "SELECT (SELECT MIN(begin_timestamp) FROM `{archives}`) AS begin_timestamp, \
                     (SELECT MAX(end_timestamp) FROM `{archives}`) AS end_timestamp, (SELECT \
                     COUNT(DISTINCT orig_file_id) FROM `{files}`) AS num_files, (SELECT \
                     CAST(COALESCE(SUM(num_messages), 0) AS SIGNED) FROM `{files}`) AS \
                     num_messages"
                )
            }
            StorageEngine::ClpS => {
                let datasets = self.get_dataset_names().await?;
                if datasets.is_empty() {
                    return Ok(None);
                }
                let archives_union = union_all(datasets.iter().map(|dataset| {
                    format!(
                        "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS \
                         end_timestamp FROM `{}`",
                        database.archives_table_name(dataset)
                    )
                }));
                let files_union = union_all(datasets.iter().map(|dataset| {
                    format!(
                        "SELECT COUNT(DISTINCT orig_file_id) AS num_files, \
                         CAST(COALESCE(SUM(num_messages), 0) AS SIGNED) AS num_messages FROM `{}`",
                        database.files_table_name(dataset)
                    )
                }));
                format!(
                    "SELECT a.begin_timestamp, a.end_timestamp, f.num_files, f.num_messages FROM \
                     (SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS \
                     end_timestamp FROM ({archives_union}) AS archives_combined) AS a, (SELECT \
                     CAST(SUM(num_files) AS SIGNED) AS num_files, CAST(SUM(num_messages) AS \
                     SIGNED) AS num_messages FROM ({files_union}) AS files_combined) AS f"
                )
            }
        };
        let row = sqlx::query(&sql).fetch_one(&self.sql_pool).await?;
        Ok(IngestionDetails::try_from(row).ok())
    }

    /// Fetches the query speed (total uncompressed bytes scanned and job duration) for a search
    /// job across the given datasets.
    ///
    /// For the CLP storage engine, `datasets` is ignored.
    ///
    /// # Returns
    ///
    /// The query-speed statistics on success, or `None` when the job hasn't scanned any archives
    /// or hasn't finished yet (or, for CLP-S, when `datasets` is empty).
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::fetch_optional`]'s return values on failure.
    pub async fn get_query_speed(
        &self,
        datasets: &[DatasetName],
        search_job_id: i64,
    ) -> Result<Option<QuerySpeed>, ClientError> {
        let database = &self.config.database;
        let archives_subquery = match self.config.package.storage_engine {
            StorageEngine::Clp => format!(
                "SELECT id, uncompressed_size FROM `{}`",
                database.unscoped_archives_table_name()
            ),
            StorageEngine::ClpS => {
                if datasets.is_empty() {
                    return Ok(None);
                }
                union_all(datasets.iter().map(|dataset| {
                    format!(
                        "SELECT id, uncompressed_size FROM `{}`",
                        database.archives_table_name(dataset)
                    )
                }))
            }
        };
        let sql = format!(
            "WITH qt AS ( SELECT job_id, archive_id FROM `{QUERY_TASKS_TABLE_NAME}` WHERE \
             archive_id IS NOT NULL AND job_id = ? ), totals AS ( SELECT qt.job_id, \
             SUM(ca.uncompressed_size) AS total_uncompressed_bytes FROM qt JOIN \
             ({archives_subquery}) ca ON qt.archive_id = ca.id ) SELECT \
             CAST(totals.total_uncompressed_bytes AS DOUBLE) AS bytes, qj.duration AS duration \
             FROM `{QUERY_JOBS_TABLE_NAME}` qj JOIN totals ON totals.job_id = qj.id"
        );
        let Some(row) = sqlx::query(&sql)
            .bind(search_job_id)
            .fetch_optional(&self.sql_pool)
            .await?
        else {
            return Ok(None);
        };
        Ok(QuerySpeed::try_from(row).ok())
    }

    /// Fetches the timestamp column names for a given dataset (CLP-S only).
    ///
    /// # Returns
    ///
    /// The distinct timestamp column names, ordered by name, on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::DatasetNotFound`] if the dataset's column metadata table doesn't exist.
    /// * Forwards [`sqlx::query::QueryScalar::fetch_all`]'s return values on failure.
    pub async fn get_timestamp_column_names(
        &self,
        dataset: &DatasetName,
    ) -> Result<Vec<String>, ClientError> {
        // Must be kept in sync with `NodeType::Timestamp` and `NodeType::DeprecatedDateString` in
        // `components/core/src/clp_s/SchemaTree.hpp`.
        const TIMESTAMP_NODE_TYPE: i8 = 14;
        const DEPRECATED_TIMESTAMP_NODE_TYPE: i8 = 8;
        // MySQL error number for "Table doesn't exist".
        const MYSQL_TABLE_NOT_FOUND: u16 = 1146;

        let table_name = self.config.database.column_metadata_table_name(dataset);
        let names: Vec<String> = sqlx::query_scalar(&format!(
            "SELECT DISTINCT name FROM `{table_name}` WHERE type IN (?, ?) ORDER BY name"
        ))
        .bind(TIMESTAMP_NODE_TYPE)
        .bind(DEPRECATED_TIMESTAMP_NODE_TYPE)
        .fetch_all(&self.sql_pool)
        .await
        .map_err(|err| {
            if let sqlx::Error::Database(db_err) = &err
                && let Some(mysql_err) =
                    db_err.try_downcast_ref::<sqlx::mysql::MySqlDatabaseError>()
                && mysql_err.number() == MYSQL_TABLE_NOT_FOUND
            {
                return ClientError::DatasetNotFound(dataset.clone());
            }
            err.into()
        })?;
        Ok(names)
    }

    /// Fetches recent compression-job metadata, with the decoded CLP IO config for each job.
    ///
    /// # Returns
    ///
    /// The compression-job metadata of the most recent jobs, most recent first, on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::TaskJoin`] if the task decoding the rows panics.
    /// * Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure.
    /// * Forwards [`CompressionMetadata::try_from`]'s return values on failure.
    pub async fn get_compression_metadata(&self) -> Result<Vec<CompressionMetadata>, ClientError> {
        const QUERY_LIMIT: i64 = 1000;

        let rows = sqlx::query(&format!(
            "SELECT id, status, status_msg, start_time, update_time, duration, uncompressed_size, \
             compressed_size, clp_config FROM `{COMPRESSION_JOBS_TABLE_NAME}` ORDER BY id DESC \
             LIMIT ?"
        ))
        .bind(QUERY_LIMIT)
        .fetch_all(&self.sql_pool)
        .await?;
        tokio::task::spawn_blocking(move || {
            rows.into_iter()
                .map(CompressionMetadata::try_from)
                .collect()
        })
        .await?
    }
}

/// Selects the job config fields exposed in the compression history.
///
/// # Returns
///
/// `config` with only the display fields of its `input` and `output` sections.
fn compression_metadata_config(mut config: serde_json::Value) -> serde_json::Value {
    // Match the display fields in `ClpIoPartialConfigSchema`. Stored input configs also contain AWS
    // credentials, which must never be returned to the browser.
    let input_fields = [
        "type",
        "dataset",
        "timestamp_key",
        "unstructured",
        "paths_to_compress",
        "path_prefix_to_remove",
        "keys",
    ];
    let output_fields = [
        "compression_level",
        "target_archive_size",
        "target_dictionaries_size",
        "target_encoded_file_size",
        "target_segment_size",
    ];
    let mut displayed = serde_json::Map::new();
    for (section, fields) in [
        ("input", input_fields.as_slice()),
        ("output", output_fields.as_slice()),
    ] {
        if let Some(values) = config
            .get_mut(section)
            .and_then(serde_json::Value::as_object_mut)
        {
            values.retain(|name, _| fields.contains(&name.as_str()));
            displayed.insert(
                section.to_owned(),
                serde_json::Value::Object(std::mem::take(values)),
            );
        }
    }
    serde_json::Value::Object(displayed)
}

/// Combines `SELECT` statements with `UNION ALL`.
///
/// # Returns
///
/// The combined statement.
fn union_all(selects: impl Iterator<Item = String>) -> String {
    selects.collect::<Vec<_>>().join("\nUNION ALL\n")
}

#[cfg(test)]
mod tests {
    use super::CompressionMetadataDisplayConfig;
    use super::compression_metadata_config;

    #[test]
    fn compression_history_omits_storage_credentials() -> anyhow::Result<()> {
        for input_type in ["s3", "s3_object_metadata"] {
            let config = serde_json::json!({
                "input": {
                    "type": input_type,
                    "dataset": "logs",
                    "timestamp_key": null,
                    "unstructured": false,
                    "aws_authentication": {"credentials": {
                        "access_key_id": "test-key",
                        "secret_access_key": "test-secret",
                        "session_token": "test-token"
                    }},
                    "bucket": "private-bucket"
                },
                "output": {"compression_level": 3},
                "private_field": "private-value"
            });
            let display = compression_metadata_config(config);
            let parsed: CompressionMetadataDisplayConfig = serde_json::from_value(display.clone())?;
            assert_eq!(parsed.input.r#type.as_deref(), Some(input_type));
            assert_eq!(
                display,
                serde_json::json!({
                    "input": {
                        "type": input_type,
                        "dataset": "logs",
                        "timestamp_key": null,
                        "unstructured": false
                    },
                    "output": {"compression_level": 3}
                })
            );
        }
        Ok(())
    }

    #[test]
    fn compression_history_preserves_partial_filesystem_config() -> anyhow::Result<()> {
        let config = serde_json::json!({
            "input": {
                "type": "fs",
                "paths_to_compress": ["/mnt/logs/app.log"],
                "path_prefix_to_remove": null
            },
            "output": {}
        });
        let display = compression_metadata_config(config.clone());
        let parsed: CompressionMetadataDisplayConfig = serde_json::from_value(display.clone())?;
        assert_eq!(
            parsed.input.paths_to_compress,
            Some(vec!["/mnt/logs/app.log".to_owned()])
        );
        assert_eq!(display, config);
        Ok(())
    }
}
