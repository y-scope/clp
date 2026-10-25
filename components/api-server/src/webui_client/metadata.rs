//! Read-only metadata about the CLP package's datasets, archives, and jobs.

use std::collections::HashSet;

use chrono::DateTime;
use chrono::Utc;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::COMPRESSION_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::CompressionJobId;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QUERY_TASKS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::serde::ZstdMsgpack;
use serde::Serialize;
use utoipa::ToSchema;

use crate::error::ClientError;
use crate::webui_client::WebuiClient;

/// `MySQL` error number for "Table doesn't exist".
const MYSQL_TABLE_NOT_FOUND: u16 = 1146;

/// Earliest and latest log entry timestamps across the selected datasets.
#[derive(Debug, PartialEq, Eq, Serialize, ToSchema)]
pub struct TimeRange {
    /// Earliest log entry timestamp (epoch milliseconds).
    pub begin_timestamp: i64,
    /// Latest log entry timestamp (epoch milliseconds).
    pub end_timestamp: i64,
}

/// Aggregated space-savings statistics.
#[derive(Debug, Serialize, sqlx::FromRow, ToSchema)]
pub struct SpaceSavings {
    /// Total uncompressed size of all archives, in bytes.
    pub total_uncompressed_size: i64,
    /// Total compressed size of all archives, in bytes.
    pub total_compressed_size: i64,
}

/// Ingestion details statistics.
#[derive(Debug, PartialEq, Eq, Serialize, ToSchema)]
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

/// Query-speed statistics for a query job.
#[derive(Debug, PartialEq, Serialize, ToSchema)]
pub struct QuerySpeed {
    /// Total uncompressed size of the archives the job scanned, in bytes.
    pub bytes: f64,
    /// Wall-clock duration the job ran, in seconds.
    pub duration: f32,
}

/// The metadata of a compression job, with the display fields of its config.
#[derive(Debug, Serialize, ToSchema)]
pub struct CompressionMetadata {
    /// The compression job's ID. Named `_id` to match the `WebUI`'s existing JSON contract.
    #[serde(rename = "_id")]
    #[schema(value_type = i32)]
    pub id: CompressionJobId,
    /// Current status of the job, as the integer value of a
    /// [`clp_rust_utils::job_config::CompressionJobStatus`].
    pub status: i32,
    /// Status message for the job.
    pub status_msg: String,
    /// Time the job started executing (RFC 3339). `null` if the job hasn't started.
    pub start_time: Option<DateTime<Utc>>,
    /// Time the job was last updated (RFC 3339).
    pub update_time: DateTime<Utc>,
    /// Wall-clock duration the job ran, in seconds. `null` if the job did not complete.
    pub duration: Option<f32>,
    /// Total uncompressed size of input files, in bytes.
    pub uncompressed_size: i64,
    /// Total compressed archive size, in bytes.
    pub compressed_size: i64,
    /// The display fields of the job's stored config. Storage credentials are omitted.
    #[schema(value_type = CompressionMetadataDisplayConfig)]
    pub clp_config: serde_json::Value,
}

/// The `OpenAPI` schema of the display subset of a stored compression job config, as returned in
/// [`CompressionMetadata::clp_config`].
#[derive(Debug, ToSchema)]
pub struct CompressionMetadataDisplayConfig {
    /// The display fields of the config's `input` section. Empty if the stored config has no
    /// `input` section, or if it isn't an object.
    pub input: CompressionMetadataDisplayInput,
    /// The display fields of the config's `output` section. Empty if the stored config has no
    /// `output` section, or if it isn't an object.
    pub output: CompressionMetadataDisplayOutput,
}

/// The `OpenAPI` schema of the display fields of a stored config's `input` section. Older
/// compression jobs may have stored only some of them.
#[derive(Debug, ToSchema)]
pub struct CompressionMetadataDisplayInput {
    /// The input type (e.g., `fs` or `s3`).
    pub r#type: Option<String>,
    /// The dataset the logs were compressed into.
    pub dataset: Option<String>,
    /// The filesystem paths to compress.
    pub paths_to_compress: Option<Vec<String>>,
    /// The prefix removed from the compressed file paths.
    pub path_prefix_to_remove: Option<String>,
    /// The S3 object keys to compress.
    pub keys: Option<Vec<String>>,
    /// The key of the log events' timestamp field.
    pub timestamp_key: Option<String>,
    /// Whether the logs are unstructured.
    pub unstructured: Option<bool>,
}

/// The `OpenAPI` schema of the display fields of a stored config's `output` section. Older
/// compression jobs may have stored only some of them.
#[derive(Debug, ToSchema)]
pub struct CompressionMetadataDisplayOutput {
    /// The compression level.
    pub compression_level: Option<u8>,
    /// The target archive size, in bytes.
    pub target_archive_size: Option<u64>,
    /// The target dictionaries size, in bytes.
    pub target_dictionaries_size: Option<u64>,
    /// The target encoded file size, in bytes.
    pub target_encoded_file_size: Option<u64>,
    /// The target segment size, in bytes.
    pub target_segment_size: Option<u64>,
}

impl WebuiClient {
    /// Fetches all dataset names from the datasets table.
    ///
    /// The CLP storage engine has no datasets table, so no dataset names are returned for it.
    ///
    /// # Returns
    ///
    /// The dataset names, ordered by name, on success, or an empty list for the CLP storage engine.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryScalar::fetch_all`]'s return values on failure.
    pub async fn get_dataset_names(&self) -> Result<Vec<DatasetName>, ClientError> {
        if self.config.package.storage_engine == StorageEngine::Clp {
            return Ok(Vec::new());
        }
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
    /// The earliest and latest log entry timestamps on success, or `None` when the datasets have no
    /// archives (including, for CLP-S, when `datasets` is empty).
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::DatasetNotFound`] if, for CLP-S, any of `datasets` doesn't exist. The first
    ///   missing dataset in `datasets` is reported.
    /// * Forwards [`sqlx::query::QueryAs::fetch_one`]'s return values on any other failure.
    /// * Forwards [`sqlx::query::QueryScalar::fetch_all`]'s return values if looking up the missing
    ///   dataset fails.
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
        let row = match sqlx::query_as::<_, TimeRangeRowProjection>(&sql)
            .fetch_one(&self.sql_pool)
            .await
        {
            Ok(row) => row,
            Err(err) => return Err(self.resolve_dataset_query_error(datasets, err).await),
        };
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
    /// * Forwards [`sqlx::query::QueryAs::fetch_one`]'s return values on failure, including when
    ///   the tables of a dataset that's being deleted no longer exist.
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
    /// * Forwards [`sqlx::query::QueryAs::fetch_one`]'s return values on failure, including when
    ///   the tables of a dataset that's being deleted no longer exist.
    pub async fn get_ingestion_details(&self) -> Result<Option<IngestionDetails>, ClientError> {
        let database = &self.config.database;
        let sql = match self.config.package.storage_engine {
            StorageEngine::Clp => {
                let archives = database.unscoped_archives_table_name();
                let files = database.unscoped_files_table_name();
                format!(
                    "SELECT a.begin_timestamp, a.end_timestamp, f.num_files, f.num_messages FROM \
                     (SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS \
                     end_timestamp FROM `{archives}`) AS a, (SELECT COUNT(DISTINCT orig_file_id) \
                     AS num_files, CAST(COALESCE(SUM(num_messages), 0) AS SIGNED) AS num_messages \
                     FROM `{files}`) AS f"
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
    /// The query-speed statistics on success, or `None` when:
    ///
    /// * the job doesn't exist;
    /// * the job has no tasks over the archives of the given datasets;
    /// * the job hasn't finished yet (its duration is `NULL`);
    /// * for CLP-S, `datasets` is empty.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::DatasetNotFound`] if, for CLP-S, any of `datasets` doesn't exist. The first
    ///   missing dataset in `datasets` is reported.
    /// * Forwards [`sqlx::query::QueryAs::fetch_optional`]'s return values on any other failure.
    /// * Forwards [`sqlx::query::QueryScalar::fetch_all`]'s return values if looking up the missing
    ///   dataset fails.
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
            "SELECT CAST(SUM(ca.uncompressed_size) AS DOUBLE) AS bytes, qj.duration AS duration \
             FROM `{QUERY_JOBS_TABLE_NAME}` qj JOIN `{QUERY_TASKS_TABLE_NAME}` qt ON qt.job_id = \
             qj.id JOIN ({archives_subquery}) ca ON qt.archive_id = ca.id WHERE qj.id = ? GROUP \
             BY qj.id, qj.duration"
        );
        let row = match sqlx::query_as::<_, QuerySpeedRowProjection>(&sql)
            .bind(query_job_id)
            .fetch_optional(&self.sql_pool)
            .await
        {
            Ok(row) => row,
            Err(err) => return Err(self.resolve_dataset_query_error(datasets, err).await),
        };
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
        const DEPRECATED_DATE_STRING_NODE_TYPE: i8 = 8;

        let table_name = self.config.database.column_metadata_table_name(dataset);
        let names: Vec<String> = sqlx::query_scalar(&format!(
            "SELECT DISTINCT name FROM `{table_name}` WHERE type IN (?, ?) ORDER BY name"
        ))
        .bind(TIMESTAMP_NODE_TYPE)
        .bind(DEPRECATED_DATE_STRING_NODE_TYPE)
        .fetch_all(&self.sql_pool)
        .await
        .map_err(|err| {
            if is_table_not_found(&err) {
                return ClientError::DatasetNotFound(dataset.clone());
            }
            err.into()
        })?;
        Ok(names)
    }

    /// Fetches the metadata of the most recent compression jobs, with the display fields of each
    /// job's config.
    ///
    /// # Returns
    ///
    /// The metadata of the 1000 most recent compression jobs, most recent first, on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::TaskJoin`] if the task decoding the rows panics.
    /// * Forwards [`sqlx::query::QueryAs::fetch_all`]'s return values on failure.
    /// * Forwards [`CompressionMetadataRowProjection::into_compression_metadata`]'s return values
    ///   on failure, so the whole call fails if any of the jobs' stored configs can't be decoded.
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
                .map(CompressionMetadataRowProjection::into_compression_metadata)
                .collect()
        })
        .await?
    }

    /// Converts the failure of a query over the given datasets' tables into a [`ClientError`].
    ///
    /// # Returns
    ///
    /// * [`ClientError::DatasetNotFound`] for the first of `datasets` that isn't in the datasets
    ///   table, if `error` is a "table doesn't exist" error and the storage engine is CLP-S.
    /// * The lookup's error, if looking up `datasets` in the datasets table fails.
    /// * `error` otherwise, including when every one of `datasets` is in the datasets table.
    async fn resolve_dataset_query_error(
        &self,
        datasets: &[DatasetName],
        error: sqlx::Error,
    ) -> ClientError {
        if self.config.package.storage_engine != StorageEngine::ClpS
            || datasets.is_empty()
            || !is_table_not_found(&error)
        {
            return error.into();
        }
        let table = self.config.database.datasets_table_name();
        let placeholders = vec!["?"; datasets.len()].join(", ");
        let sql = format!("SELECT name FROM `{table}` WHERE name IN ({placeholders})");
        let mut query = sqlx::query_scalar::<_, DatasetName>(&sql);
        for dataset in datasets {
            query = query.bind(dataset);
        }
        let existing: HashSet<DatasetName> = match query.fetch_all(&self.sql_pool).await {
            Ok(names) => names.into_iter().collect(),
            Err(lookup_error) => return lookup_error.into(),
        };
        datasets
            .iter()
            .find(|dataset| !existing.contains(*dataset))
            .map_or_else(
                || error.into(),
                |dataset| ClientError::DatasetNotFound(dataset.clone()),
            )
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
    duration: Option<f32>,
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

/// A projection of a compression job row, with its config still serialized.
#[derive(Debug, sqlx::FromRow)]
struct CompressionMetadataRowProjection {
    id: CompressionJobId,
    status: i32,
    status_msg: String,
    start_time: Option<DateTime<Utc>>,
    update_time: DateTime<Utc>,
    duration: Option<f32>,
    uncompressed_size: i64,
    compressed_size: i64,
    clp_config: Vec<u8>,
}

impl CompressionMetadataRowProjection {
    /// Converts the projection into a [`CompressionMetadata`], keeping only the display fields of
    /// the job's config.
    ///
    /// # Returns
    ///
    /// The compression metadata on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`ZstdMsgpack::deserialize`]'s return values on failure.
    fn into_compression_metadata(self) -> Result<CompressionMetadata, ClientError> {
        let clp_config =
            select_compression_metadata_display_fields(ZstdMsgpack::deserialize(&self.clp_config)?);
        Ok(CompressionMetadata {
            id: self.id,
            status: self.status,
            status_msg: self.status_msg,
            start_time: self.start_time,
            update_time: self.update_time,
            duration: self.duration,
            uncompressed_size: self.uncompressed_size,
            compressed_size: self.compressed_size,
            clp_config,
        })
    }
}

/// Selects the display fields of a stored compression job config.
///
/// The selected fields must be kept in sync with [`CompressionMetadataDisplayInput`] and
/// [`CompressionMetadataDisplayOutput`].
///
/// # Returns
///
/// An object with the `input` and `output` sections of `config`, each with only its display
/// fields. A section that's missing from `config`, or isn't an object, is returned as an empty
/// object.
fn select_compression_metadata_display_fields(mut config: serde_json::Value) -> serde_json::Value {
    // Match the display fields in `ClpIoPartialConfigSchema`. Stored input configs also contain AWS
    // credentials, which must never be returned to the browser.
    const INPUT_DISPLAY_FIELDS: &[&str] = &[
        "type",
        "dataset",
        "timestamp_key",
        "unstructured",
        "paths_to_compress",
        "path_prefix_to_remove",
        "keys",
    ];
    const OUTPUT_DISPLAY_FIELDS: &[&str] = &[
        "compression_level",
        "target_archive_size",
        "target_dictionaries_size",
        "target_encoded_file_size",
        "target_segment_size",
    ];

    let mut displayed = serde_json::Map::new();
    for (section, fields) in [
        ("input", INPUT_DISPLAY_FIELDS),
        ("output", OUTPUT_DISPLAY_FIELDS),
    ] {
        let mut values = config
            .get_mut(section)
            .and_then(serde_json::Value::as_object_mut)
            .map_or_else(serde_json::Map::new, std::mem::take);
        values.retain(|name, _| fields.contains(&name.as_str()));
        displayed.insert(section.to_owned(), serde_json::Value::Object(values));
    }
    serde_json::Value::Object(displayed)
}

/// # Returns
///
/// Whether `error` is a `MySQL` "table doesn't exist" error.
fn is_table_not_found(error: &sqlx::Error) -> bool {
    if let sqlx::Error::Database(db_err) = error
        && let Some(mysql_err) = db_err.try_downcast_ref::<sqlx::mysql::MySqlDatabaseError>()
    {
        return mysql_err.number() == MYSQL_TABLE_NOT_FOUND;
    }
    false
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
    use chrono::DateTime;
    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::clp_config::AwsCredentials;
    use clp_rust_utils::clp_config::S3Config;
    use clp_rust_utils::clp_config::package::config::Config;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::job_config::ClpIoConfig;
    use clp_rust_utils::job_config::InputConfig;
    use clp_rust_utils::job_config::OutputConfig;
    use clp_rust_utils::job_config::S3InputConfig;
    use clp_rust_utils::serde::ZstdMsgpack;
    use non_empty_string::NonEmptyString;

    use super::CompressionMetadataRowProjection;
    use super::IngestionDetails;
    use super::IngestionDetailsRowProjection;
    use super::QuerySpeed;
    use super::QuerySpeedRowProjection;
    use super::TimeRange;
    use super::TimeRangeRowProjection;
    use super::select_compression_metadata_display_fields;
    use crate::webui_client::WebuiClient;

    fn non_empty(value: &str) -> NonEmptyString {
        NonEmptyString::new(value.to_owned()).expect("the test value should be non-empty")
    }

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

    #[tokio::test]
    async fn clp_dataset_names_are_empty() -> anyhow::Result<()> {
        let config = Config {
            package: Package {
                storage_engine: StorageEngine::Clp,
            },
            ..Config::default()
        };
        let sql_pool = sqlx::mysql::MySqlPoolOptions::new()
            .acquire_timeout(std::time::Duration::from_secs(1))
            .connect_lazy("mysql://localhost:1/clp-db")?;
        let client = WebuiClient::new(config, sql_pool);
        assert_eq!(client.get_dataset_names().await?, Vec::<DatasetName>::new());
        Ok(())
    }

    #[tokio::test]
    async fn clp_s_query_speed_without_datasets_is_none() -> anyhow::Result<()> {
        let config = Config {
            package: Package {
                storage_engine: StorageEngine::ClpS,
            },
            ..Config::default()
        };
        let sql_pool = sqlx::mysql::MySqlPoolOptions::new()
            .acquire_timeout(std::time::Duration::from_secs(1))
            .connect_lazy("mysql://localhost:1/clp-db")?;
        let client = WebuiClient::new(config, sql_pool);
        assert_eq!(client.get_query_speed(&[], 1).await?, None);
        Ok(())
    }

    #[test]
    fn compression_metadata_keeps_display_fields_and_omits_storage_credentials()
    -> anyhow::Result<()> {
        let config = ClpIoConfig {
            input: InputConfig::S3InputConfig {
                config: S3InputConfig {
                    s3_config: S3Config {
                        bucket: non_empty("the-bucket"),
                        region_code: Some(non_empty("us-east-2")),
                        key_prefix: non_empty("logs/"),
                        endpoint_url: None,
                        aws_authentication: AwsAuthentication::Credentials {
                            credentials: AwsCredentials {
                                access_key_id: "the-access-key".to_owned(),
                                secret_access_key: "the-secret-key".to_owned(),
                                session_token: Some("the-session-token".to_owned()),
                            },
                        },
                    },
                    keys: Some(vec![non_empty("logs/app.log")]),
                    dataset: "logs".parse()?,
                    timestamp_key: Some(non_empty("ts")),
                    unstructured: false,
                },
            },
            output: OutputConfig {
                target_archive_size: 1,
                target_dictionaries_size: 2,
                target_encoded_file_size: 3,
                target_segment_size: 4,
                compression_level: 5,
            },
        };
        let start_time = DateTime::from_timestamp(1_700_000_000, 0).expect("valid timestamp");
        let update_time = DateTime::from_timestamp(1_700_000_060, 0).expect("valid timestamp");
        let row = CompressionMetadataRowProjection {
            id: 7,
            status: 2,
            status_msg: "done".to_owned(),
            start_time: Some(start_time),
            update_time,
            duration: Some(60.0),
            uncompressed_size: 100,
            compressed_size: 10,
            clp_config: ZstdMsgpack::serialize(&config)?,
        };

        let metadata = row.into_compression_metadata()?;

        assert_eq!(metadata.id, 7);
        assert_eq!(metadata.status, 2);
        assert_eq!(metadata.status_msg, "done");
        assert_eq!(metadata.start_time, Some(start_time));
        assert_eq!(metadata.update_time, update_time);
        assert_eq!(metadata.duration, Some(60.0));
        assert_eq!(metadata.uncompressed_size, 100);
        assert_eq!(metadata.compressed_size, 10);
        assert_eq!(
            metadata.clp_config,
            serde_json::json!({
                "input": {
                    "type": "s3",
                    "dataset": "logs",
                    "keys": ["logs/app.log"],
                    "timestamp_key": "ts",
                    "unstructured": false
                },
                "output": {
                    "target_archive_size": 1,
                    "target_dictionaries_size": 2,
                    "target_encoded_file_size": 3,
                    "target_segment_size": 4,
                    "compression_level": 5
                }
            })
        );
        Ok(())
    }

    #[test]
    fn compression_metadata_display_fields_default_missing_or_non_object_sections_to_empty() {
        let output = serde_json::json!({"compression_level": 3});
        for (config, expected) in [
            (
                serde_json::json!({}),
                serde_json::json!({"input": {}, "output": {}}),
            ),
            (
                serde_json::json!({"output": output}),
                serde_json::json!({"input": {}, "output": output}),
            ),
            (
                serde_json::json!({"input": null, "output": output}),
                serde_json::json!({"input": {}, "output": output}),
            ),
            (
                serde_json::json!({"input": {"type": "fs"}, "output": [3]}),
                serde_json::json!({"input": {"type": "fs"}, "output": {}}),
            ),
            (
                serde_json::json!({"input": {"bucket": "the-bucket"}, "output": {}}),
                serde_json::json!({"input": {}, "output": {}}),
            ),
        ] {
            assert_eq!(
                select_compression_metadata_display_fields(config.clone()),
                expected,
                "config: {config}"
            );
        }
    }

    #[test]
    fn compression_metadata_display_fields_preserve_partial_filesystem_config() {
        let config = serde_json::json!({
            "input": {
                "type": "fs",
                "paths_to_compress": ["/mnt/logs/app.log"],
                "path_prefix_to_remove": null
            },
            "output": {"compression_level": 3}
        });
        assert_eq!(
            select_compression_metadata_display_fields(config.clone()),
            config
        );
    }
}
