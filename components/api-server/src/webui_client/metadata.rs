//! Read-only metadata about the CLP package's datasets, archives, and jobs.

use chrono::DateTime;
use chrono::Utc;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::COMPRESSION_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QUERY_TASKS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::serde::ZstdMsgpack;
use serde::Deserialize;
use serde::Serialize;
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

/// Aggregated space-savings statistics.
#[derive(Clone, Debug, Serialize, Deserialize, sqlx::FromRow, ToSchema)]
pub struct SpaceSavings {
    /// Total uncompressed size of all archives, in bytes.
    pub total_uncompressed_size: i64,
    /// Total compressed size of all archives, in bytes.
    pub total_compressed_size: i64,
}

/// Ingestion details statistics. The whole value is `None` when no data has been ingested yet.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, ToSchema)]
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

/// Query-speed statistics for a query job. The whole value is `None` until the job has scanned
/// archives and finished (its duration is recorded only on completion).
#[derive(Clone, Debug, PartialEq, Serialize, Deserialize, ToSchema)]
pub struct QuerySpeed {
    /// Total uncompressed size of the archives the job scanned, in bytes.
    pub bytes: f64,
    /// Wall-clock duration the job ran, in seconds.
    pub duration: f64,
}

/// Earliest and latest log entry timestamps across the selected datasets. The whole value is `None`
/// when no archives exist yet (or, for CLP-S, when no datasets were selected).
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, ToSchema)]
pub struct TimeRange {
    /// Earliest log entry timestamp (epoch milliseconds).
    pub begin_timestamp: i64,
    /// Latest log entry timestamp (epoch milliseconds).
    pub end_timestamp: i64,
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
    /// * Forwards [`sqlx::query::QueryAs::fetch_one`]'s return values on failure.
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
        let row = sqlx::query_as::<_, TimeRangeRowProjection>(&sql)
            .fetch_one(&self.sql_pool)
            .await?;
        Ok(row.into_time_range())
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
    /// * Forwards [`sqlx::query::QueryAs::fetch_one`]'s return values on failure.
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
        let space_savings = sqlx::query_as::<_, SpaceSavings>(&sql)
            .fetch_one(&self.sql_pool)
            .await?;
        Ok(space_savings)
    }

    /// Fetches ingestion details (timestamp range, file count, message count) across all datasets.
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
    /// * Forwards [`sqlx::query::QueryAs::fetch_one`]'s return values on failure.
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
        let row = sqlx::query_as::<_, IngestionDetailsRowProjection>(&sql)
            .fetch_one(&self.sql_pool)
            .await?;
        Ok(row.into_ingestion_details())
    }

    /// Fetches the query speed (total uncompressed bytes scanned and job duration) for a query job
    /// across the given datasets.
    ///
    /// For the CLP storage engine, `datasets` is ignored.
    ///
    /// # Returns
    ///
    /// The query-speed statistics on success, or `None` when the job hasn't scanned any archives or
    /// hasn't finished yet (or, for CLP-S, when `datasets` is empty).
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_optional`]'s return values on failure.
    pub async fn get_query_speed(
        &self,
        datasets: &[DatasetName],
        query_job_id: QueryJobId,
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
        let row = sqlx::query_as::<_, QuerySpeedRowProjection>(&sql)
            .bind(query_job_id)
            .fetch_optional(&self.sql_pool)
            .await?;
        Ok(row.and_then(QuerySpeedRowProjection::into_query_speed))
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
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    /// * Forwards [`CompressionMetadata::try_from`]'s return values on failure.
    pub async fn get_compression_metadata(&self) -> Result<Vec<CompressionMetadata>, ClientError> {
        const QUERY_LIMIT: i64 = 1000;

        let rows = sqlx::query_as::<_, CompressionMetadataRowProjection>(&format!(
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

/// A projection of a compression job row, with its CLP IO config still serialized.
#[derive(Debug, sqlx::FromRow)]
struct CompressionMetadataRowProjection {
    id: i64,
    status: i32,
    status_msg: String,
    start_time: Option<DateTime<Utc>>,
    update_time: DateTime<Utc>,
    duration: Option<f64>,
    uncompressed_size: i64,
    compressed_size: i64,
    clp_config: Vec<u8>,
}

impl TryFrom<CompressionMetadataRowProjection> for CompressionMetadata {
    type Error = ClientError;

    fn try_from(row: CompressionMetadataRowProjection) -> Result<Self, Self::Error> {
        let clp_config = compression_metadata_config(ZstdMsgpack::deserialize(&row.clp_config)?);
        Ok(Self {
            id: row.id,
            status: row.status,
            status_msg: row.status_msg,
            start_time: row.start_time.map(|time| time.to_rfc3339()),
            update_time: row.update_time.to_rfc3339(),
            duration: row.duration,
            uncompressed_size: row.uncompressed_size,
            compressed_size: row.compressed_size,
            clp_config,
        })
    }
}

/// A projection of the earliest and latest log entry timestamps aggregated over archives. Both are
/// `NULL` when there are no archives to aggregate.
#[derive(Debug, sqlx::FromRow)]
struct TimeRangeRowProjection {
    begin_timestamp: Option<i64>,
    end_timestamp: Option<i64>,
}

impl TimeRangeRowProjection {
    /// Converts the projection into a [`TimeRange`].
    ///
    /// # Returns
    ///
    /// The time range, or `None` if either timestamp is `NULL`.
    const fn into_time_range(self) -> Option<TimeRange> {
        let (Some(begin_timestamp), Some(end_timestamp)) =
            (self.begin_timestamp, self.end_timestamp)
        else {
            return None;
        };
        Some(TimeRange {
            begin_timestamp,
            end_timestamp,
        })
    }
}

/// A projection of the ingestion details aggregated over archives and files. The timestamps are
/// `NULL` when there are no archives to aggregate.
#[derive(Debug, sqlx::FromRow)]
struct IngestionDetailsRowProjection {
    begin_timestamp: Option<i64>,
    end_timestamp: Option<i64>,
    num_files: i64,
    num_messages: i64,
}

impl IngestionDetailsRowProjection {
    /// Converts the projection into an [`IngestionDetails`].
    ///
    /// # Returns
    ///
    /// The ingestion details, or `None` if either timestamp is `NULL`.
    const fn into_ingestion_details(self) -> Option<IngestionDetails> {
        let (Some(begin_timestamp), Some(end_timestamp)) =
            (self.begin_timestamp, self.end_timestamp)
        else {
            return None;
        };
        Some(IngestionDetails {
            begin_timestamp,
            end_timestamp,
            num_files: self.num_files,
            num_messages: self.num_messages,
        })
    }
}

/// A projection of a query job's scanned bytes and duration. The duration is `NULL` until the job
/// finishes.
#[derive(Debug, sqlx::FromRow)]
struct QuerySpeedRowProjection {
    bytes: f64,
    duration: Option<f64>,
}

impl QuerySpeedRowProjection {
    /// Converts the projection into a [`QuerySpeed`].
    ///
    /// # Returns
    ///
    /// The query speed, or `None` if the duration is `NULL`.
    const fn into_query_speed(self) -> Option<QuerySpeed> {
        let Some(duration) = self.duration else {
            return None;
        };
        Some(QuerySpeed {
            bytes: self.bytes,
            duration,
        })
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
    use super::IngestionDetails;
    use super::IngestionDetailsRowProjection;
    use super::QuerySpeed;
    use super::QuerySpeedRowProjection;
    use super::TimeRange;
    use super::TimeRangeRowProjection;
    use super::compression_metadata_config;

    #[test]
    fn time_range_is_none_unless_both_timestamps_are_present() {
        for (begin_timestamp, end_timestamp) in [(None, None), (Some(1), None), (None, Some(2))] {
            let row = TimeRangeRowProjection {
                begin_timestamp,
                end_timestamp,
            };
            assert_eq!(row.into_time_range(), None);
        }
        let row = TimeRangeRowProjection {
            begin_timestamp: Some(1),
            end_timestamp: Some(2),
        };
        assert_eq!(
            row.into_time_range(),
            Some(TimeRange {
                begin_timestamp: 1,
                end_timestamp: 2,
            })
        );
    }

    #[test]
    fn ingestion_details_is_none_unless_both_timestamps_are_present() {
        for (begin_timestamp, end_timestamp) in [(None, None), (Some(1), None), (None, Some(2))] {
            let row = IngestionDetailsRowProjection {
                begin_timestamp,
                end_timestamp,
                num_files: 0,
                num_messages: 0,
            };
            assert_eq!(row.into_ingestion_details(), None);
        }
        let row = IngestionDetailsRowProjection {
            begin_timestamp: Some(1),
            end_timestamp: Some(2),
            num_files: 3,
            num_messages: 4,
        };
        assert_eq!(
            row.into_ingestion_details(),
            Some(IngestionDetails {
                begin_timestamp: 1,
                end_timestamp: 2,
                num_files: 3,
                num_messages: 4,
            })
        );
    }

    #[test]
    fn query_speed_is_none_until_the_duration_is_recorded() {
        let row = QuerySpeedRowProjection {
            bytes: 1024.0,
            duration: None,
        };
        assert_eq!(row.into_query_speed(), None);
        let row = QuerySpeedRowProjection {
            bytes: 1024.0,
            duration: Some(2.0),
        };
        assert_eq!(
            row.into_query_speed(),
            Some(QuerySpeed {
                bytes: 1024.0,
                duration: 2.0,
            })
        );
    }

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
