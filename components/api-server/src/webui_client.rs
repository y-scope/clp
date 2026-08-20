use std::path::PathBuf;
use std::time::Duration;

use aws_sdk_s3::presigning::PresigningConfig;
use chrono::DateTime;
use chrono::Utc;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::S3Config;
use clp_rust_utils::clp_config::package::config::Config;
use clp_rust_utils::clp_config::package::config::LogsInput;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
use clp_rust_utils::clp_config::package::credentials::Credentials;
use clp_rust_utils::database::mysql::create_clp_db_mysql_pool;
use clp_rust_utils::dataset::VALID_DATASET_NAME_REGEX;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::serde::ZstdMsgpack;
use mongodb::bson::doc;
use num_enum::IntoPrimitive;
use num_enum::TryFromPrimitive;
use serde::Deserialize;
use serde::Serialize;
use sqlx::Row;
use utoipa::ToSchema;

use crate::error::ClientError;

/// Mirror of the extract variants of `job_orchestration.scheduler.constants.QueryJobType`.
/// Must be kept in sync with [`clp_rust_utils::job_config::QueryJobType`].
#[derive(
    Clone,
    Copy,
    Debug,
    Deserialize,
    Eq,
    PartialEq,
    Serialize,
    ToSchema,
    TryFromPrimitive,
    IntoPrimitive,
)]
#[repr(i32)]
pub enum ExtractJobType {
    ExtractIr = 1,
    ExtractJson = 2,
}

/// Schema mirror of `NodeType::DeprecatedDateString` in
/// `components/core/src/clp_s/SchemaTree.hpp`.
const DEPRECATED_TIMESTAMP_TYPE: i8 = 8;

/// Schema mirror of `NodeType::Timestamp` in `components/core/src/clp_s/SchemaTree.hpp`.
const TIMESTAMP_TYPE: i8 = 14;

/// Maximum number of compression-metadata rows to return.
const COMPRESSION_METADATA_QUERY_LIMIT: i64 = 1000;

/// Request body for submitting a compression job.
#[derive(Clone, Debug, Deserialize, Serialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct CompressionJobCreation {
    /// Absolute filesystem paths of the files to compress.
    pub paths: Vec<String>,
    /// Dataset to compress into (CLP-S only). Optional for the CLP storage engine.
    #[serde(default)]
    pub dataset: Option<String>,
    /// Timestamp key to use when parsing logs.
    #[serde(default)]
    pub timestamp_key: Option<String>,
    /// Whether the input is unstructured. Defaults to `true` for CLP and `false` for CLP-S.
    #[serde(default)]
    pub unstructured: Option<bool>,
}

/// Response body containing the ID of a newly created compression job.
#[derive(Clone, Serialize, Deserialize, ToSchema)]
pub struct CompressionJob {
    /// The ID of the newly created compression job.
    pub job_id: i64,
}

/// A row of compression metadata, with the decoded CLP IO config.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct CompressionMetadata {
    /// The compression job's ID. Named `_id` to match the webui's existing JSON contract.
    #[allow(clippy::pub_underscore_fields)]
    pub _id: i64,
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
    /// Decoded CLP IO config (as a JSON value) since the stored config is a zstd-compressed
    /// msgpack blob.
    pub clp_config: serde_json::Value,
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

    /// # Errors
    ///
    /// Returns [`sqlx::Error`] if a column cannot be decoded.
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

    /// # Errors
    ///
    /// Returns [`sqlx::Error`] if any column is NULL (i.e. no data has been ingested yet).
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

    /// # Errors
    ///
    /// Returns [`sqlx::Error`] if a column is NULL (`duration` is NULL until the job
    /// finishes).
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

    /// # Errors
    ///
    /// Returns [`sqlx::Error`] if either timestamp is NULL (i.e. no archives exist yet).
    fn try_from(row: sqlx::mysql::MySqlRow) -> Result<Self, Self::Error> {
        Ok(Self {
            begin_timestamp: row.try_get("begin_timestamp")?,
            end_timestamp: row.try_get("end_timestamp")?,
        })
    }
}

/// A directory entry returned by the file-listing endpoint.
///
/// Serialized in camelCase to match the webui's existing `FileEntry` JSON contract.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
#[serde(rename_all = "camelCase")]
pub struct DirEntry {
    /// Whether the entry is a directory or symlink that can be expanded.
    pub is_expandable: bool,
    /// The entry's file name.
    pub name: String,
    /// Path of the directory containing the entry.
    pub parent_path: String,
}

/// Extracted stream-file metadata returned by the stream-files extract endpoint.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct StreamFileMetadata {
    /// Index of the first log event in the stream file (inclusive).
    pub begin_msg_ix: i64,
    /// Index of the last log event in the stream file (exclusive).
    pub end_msg_ix: i64,
    /// Whether this is the stream's last chunk.
    pub is_last_chunk: bool,
    /// The resolved stream-file path: a pre-signed URL when stream-files S3 storage is
    /// configured, otherwise a path relative to the webui `/streams` static mount.
    pub path: String,
    /// ID of the stream the file was extracted from.
    pub stream_id: String,
}

/// Request body for the stream-files extract endpoint.
#[derive(Clone, Debug, Deserialize, Serialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamFileExtraction {
    /// Dataset the stream belongs to (CLP-S only; `null` for the CLP storage engine).
    #[serde(default)]
    pub dataset: Option<String>,
    /// The type of extraction job to submit.
    pub extract_job_type: ExtractJobType,
    /// Index of the log event the extracted stream file must contain.
    pub log_event_idx: i64,
    /// ID of the stream to extract.
    #[schema(min_length = 1)]
    pub stream_id: String,
}

/// A dedicated client for metadata, compression-job, and stream-file operations.
#[derive(Clone)]
pub struct WebuiClient {
    mongodb_client: mongodb::Client,
    sql_pool: sqlx::Pool<sqlx::MySql>,
    stream_output_s3_client: Option<aws_sdk_s3::Client>,
    config: Config,
}

impl WebuiClient {
    /// Creates a metadata client using the supplied shared database clients and the S3
    /// client for stream-output operations (`None` when stream output is filesystem-backed).
    #[must_use]
    pub fn new(
        config: &Config,
        mongodb_client: mongodb::Client,
        sql_pool: sqlx::Pool<sqlx::MySql>,
        stream_output_s3_client: Option<aws_sdk_s3::Client>,
    ) -> Self {
        Self {
            config: config.clone(),
            mongodb_client,
            sql_pool,
            stream_output_s3_client,
        }
    }

    /// Factory method to create a new [`WebuiClient`] with active connections to both
    /// `MySQL` and `MongoDB`.
    ///
    /// # Returns
    ///
    /// A newly created [`WebuiClient`] instance with active connections to both databases.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::ConfigIsNone`] if `config.api_server` is `None`.
    /// * Forwards [`create_clp_db_mysql_pool`]'s errors on failure.
    /// * Forwards [`mongodb::Client::with_uri_str`]'s errors on failure.
    pub async fn connect(config: &Config, credentials: &Credentials) -> Result<Self, ClientError> {
        if config.api_server.is_none() {
            return Err(ClientError::ConfigIsNone);
        }

        let sql_pool =
            create_clp_db_mysql_pool(&config.database, &credentials.database, 10).await?;

        let mongo_uri = format!(
            "mongodb://{}:{}/{}?directConnection=true",
            config.results_cache.host, config.results_cache.port, config.results_cache.db_name,
        );
        let mongo_client = mongodb::Client::with_uri_str(mongo_uri).await?;

        let stream_output_s3_client = match &config.stream_output.storage {
            StreamOutputStorage::S3 { s3_config, .. } => Some(
                clp_rust_utils::s3::create_new_client(
                    s3_config
                        .region_code
                        .as_ref()
                        .map_or(AWS_DEFAULT_REGION, non_empty_string::NonEmptyString::as_str),
                    s3_config.endpoint_url.as_ref(),
                    &s3_config.aws_authentication,
                )
                .await,
            ),
            StreamOutputStorage::Fs { .. } => None,
        };

        Ok(Self::new(
            config,
            mongo_client,
            sql_pool,
            stream_output_s3_client,
        ))
    }

    /// Builds a metadata table name, mirroring
    /// `clp_py_utils.clp_metadata_db_utils._get_table_name`.
    ///
    /// # Returns
    ///
    /// The table name in the form `<prefix>[<dataset>_]<suffix>`. Unlike the helpers in
    /// `clp_rust_utils`, `None` omits the dataset segment entirely (the CLP storage engine's
    /// tables, e.g. `clp_archives`, have no dataset segment) instead of resolving to the
    /// default dataset.
    fn table_name(&self, dataset: Option<&str>, suffix: &str) -> String {
        let prefix = &self.config.database.table_prefix;
        dataset.map_or_else(
            || format!("{prefix}{suffix}"),
            |dataset| format!("{prefix}{dataset}_{suffix}"),
        )
    }

    /// Builds a `UNION ALL` of per-dataset SELECT statements over each dataset's `suffix`
    /// table, validating every dataset name.
    ///
    /// # Returns
    ///
    /// The combined query string, where each branch is `select_for_table` applied to the
    /// dataset's table name.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidDatasetName`] if any dataset name is invalid.
    fn dataset_union(
        &self,
        datasets: &[String],
        suffix: &str,
        select_for_table: impl Fn(&str) -> String,
    ) -> Result<String, ClientError> {
        datasets
            .iter()
            .map(|dataset| {
                if !VALID_DATASET_NAME_REGEX.is_match(dataset) {
                    return Err(ClientError::InvalidDatasetName);
                }
                Ok(select_for_table(&self.table_name(Some(dataset), suffix)))
            })
            .collect::<Result<Vec<_>, _>>()
            .map(|selects| selects.join("\nUNION ALL\n"))
    }

    /// Fetches all dataset names from the datasets table.
    ///
    /// # Returns
    ///
    /// The dataset names, ordered by name.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure.
    pub async fn get_dataset_names(&self) -> Result<Vec<String>, ClientError> {
        let table = self.config.database.datasets_table_name();
        let names: Vec<String> =
            sqlx::query_scalar(&format!("SELECT name FROM `{table}` ORDER BY name"))
                .fetch_all(&self.sql_pool)
                .await?;
        Ok(names)
    }

    /// Fetches the earliest and latest log entry timestamps across the given datasets.
    ///
    /// For the CLP storage engine, `datasets` is ignored and the single `clp_archives` table
    /// is queried. For CLP-S, the union of the per-dataset archives tables is queried.
    ///
    /// # Returns
    ///
    /// The earliest and latest log entry timestamps, or `None` when no archives exist yet
    /// (or, for CLP-S, when `datasets` is empty).
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidDatasetName`] if any dataset name is invalid.
    /// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
    pub async fn get_time_range(
        &self,
        datasets: &[String],
    ) -> Result<Option<TimeRange>, ClientError> {
        match &self.config.package.storage_engine {
            StorageEngine::Clp => {
                let table = self.table_name(None, "archives");
                let row = sqlx::query(&format!(
                    "SELECT MIN(begin_timestamp) AS begin_timestamp, \
                     MAX(end_timestamp) AS end_timestamp FROM `{table}`"
                ))
                .fetch_one(&self.sql_pool)
                .await?;
                Ok(TimeRange::try_from(row).ok())
            }
            StorageEngine::ClpS => {
                if datasets.is_empty() {
                    return Ok(None);
                }
                let union = self.dataset_union(datasets, "archives", |table| {
                    format!(
                        "SELECT MIN(begin_timestamp) AS begin_timestamp, \
                         MAX(end_timestamp) AS end_timestamp FROM `{table}`"
                    )
                })?;
                let sql = format!(
                    "SELECT MIN(begin_timestamp) AS begin_timestamp, \
                     MAX(end_timestamp) AS end_timestamp FROM ({union}) AS combined"
                );
                let row = sqlx::query(&sql).fetch_one(&self.sql_pool).await?;
                Ok(TimeRange::try_from(row).ok())
            }
        }
    }

    /// Fetches aggregated space-savings statistics across all datasets.
    ///
    /// # Returns
    ///
    /// The total uncompressed and compressed sizes; both are `0` when no data has been
    /// ingested yet.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidDatasetName`] if a stored dataset name is invalid.
    /// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
    pub async fn get_space_savings(&self) -> Result<SpaceSavings, ClientError> {
        let sql = match &self.config.package.storage_engine {
            StorageEngine::Clp => {
                let table = self.table_name(None, "archives");
                format!(
                    "SELECT \
                       CAST(COALESCE(SUM(uncompressed_size), 0) AS SIGNED) AS total_uncompressed_size, \
                       CAST(COALESCE(SUM(size), 0) AS SIGNED) AS total_compressed_size \
                     FROM `{table}`"
                )
            }
            StorageEngine::ClpS => {
                let datasets = self.get_dataset_names().await?;
                if datasets.is_empty() {
                    return Ok(SpaceSavings {
                        total_uncompressed_size: 0,
                        total_compressed_size: 0,
                    });
                }
                let union = self.dataset_union(&datasets, "archives", |table| {
                    format!("SELECT uncompressed_size, size FROM `{table}`")
                })?;
                format!(
                    "SELECT \
                       CAST(COALESCE(SUM(uncompressed_size), 0) AS SIGNED) AS total_uncompressed_size, \
                       CAST(COALESCE(SUM(size), 0) AS SIGNED) AS total_compressed_size \
                     FROM ({union}) AS archives_combined"
                )
            }
        };
        let row = sqlx::query(&sql).fetch_one(&self.sql_pool).await?;
        Ok(row.try_into()?)
    }

    /// Fetches ingestion details (timestamp range, file count, message count) across all
    /// datasets.
    ///
    /// # Returns
    ///
    /// The ingestion details, or `None` when no data has been ingested yet.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidDatasetName`] if a stored dataset name is invalid.
    /// * Forwards [`sqlx::query::Query::fetch_one`]'s return values on failure.
    pub async fn get_ingestion_details(&self) -> Result<Option<IngestionDetails>, ClientError> {
        let sql = match &self.config.package.storage_engine {
            StorageEngine::Clp => {
                let archives = self.table_name(None, "archives");
                let files = self.table_name(None, "files");
                format!(
                    "SELECT \
                       (SELECT MIN(begin_timestamp) FROM `{archives}`) AS begin_timestamp, \
                       (SELECT MAX(end_timestamp) FROM `{archives}`) AS end_timestamp, \
                       (SELECT COUNT(DISTINCT orig_file_id) FROM `{files}`) AS num_files, \
                       (SELECT CAST(COALESCE(SUM(num_messages), 0) AS SIGNED) FROM `{files}`) \
                         AS num_messages"
                )
            }
            StorageEngine::ClpS => {
                let datasets = self.get_dataset_names().await?;
                if datasets.is_empty() {
                    return Ok(None);
                }
                let archives_union = self.dataset_union(&datasets, "archives", |table| {
                    format!(
                        "SELECT MIN(begin_timestamp) AS begin_timestamp, \
                         MAX(end_timestamp) AS end_timestamp FROM `{table}`"
                    )
                })?;
                let files_union = self.dataset_union(&datasets, "files", |table| {
                    format!(
                        "SELECT COUNT(DISTINCT orig_file_id) AS num_files, \
                         CAST(COALESCE(SUM(num_messages), 0) AS SIGNED) AS num_messages \
                         FROM `{table}`"
                    )
                })?;
                format!(
                    "SELECT a.begin_timestamp, a.end_timestamp, f.num_files, f.num_messages \
                     FROM \
                       (SELECT MIN(begin_timestamp) AS begin_timestamp, \
                               MAX(end_timestamp) AS end_timestamp \
                        FROM ({archives_union}) AS archives_combined) AS a, \
                       (SELECT CAST(SUM(num_files) AS SIGNED) AS num_files, \
                               CAST(SUM(num_messages) AS SIGNED) AS num_messages \
                        FROM ({files_union}) AS files_combined) AS f"
                )
            }
        };
        let row = sqlx::query(&sql).fetch_one(&self.sql_pool).await?;
        Ok(IngestionDetails::try_from(row).ok())
    }

    /// Fetches the query speed (total uncompressed bytes scanned and job duration) for a
    /// search job across the given datasets.
    ///
    /// # Returns
    ///
    /// The query-speed statistics, or `None` when the job hasn't scanned any archives or
    /// hasn't finished yet.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidDatasetName`] if any dataset name is invalid.
    /// * Forwards [`sqlx::query::Query::fetch_optional`]'s return values on failure.
    pub async fn get_query_speed(
        &self,
        datasets: &[String],
        search_job_id: i64,
    ) -> Result<Option<QuerySpeed>, ClientError> {
        let archives_subquery = match &self.config.package.storage_engine {
            StorageEngine::Clp => {
                let table = self.table_name(None, "archives");
                format!("SELECT id, uncompressed_size FROM `{table}`")
            }
            StorageEngine::ClpS => {
                if datasets.is_empty() {
                    return Ok(None);
                }
                self.dataset_union(datasets, "archives", |table| {
                    format!("SELECT id, uncompressed_size FROM `{table}`")
                })?
            }
        };
        let sql = format!(
            "WITH qt AS ( \
               SELECT job_id, archive_id FROM query_tasks \
               WHERE archive_id IS NOT NULL AND job_id = ? \
             ), \
             totals AS ( \
               SELECT qt.job_id, SUM(ca.uncompressed_size) AS total_uncompressed_bytes \
               FROM qt JOIN ({archives_subquery}) ca ON qt.archive_id = ca.id \
             ) \
             SELECT \
               CAST(totals.total_uncompressed_bytes AS DOUBLE) AS bytes, \
               qj.duration AS duration \
             FROM query_jobs qj JOIN totals ON totals.job_id = qj.id"
        );
        let row = sqlx::query(&sql)
            .bind(search_job_id)
            .fetch_optional(&self.sql_pool)
            .await?;
        let Some(row) = row else {
            return Ok(None);
        };
        Ok(QuerySpeed::try_from(row).ok())
    }

    /// Fetches the timestamp column names for a given dataset (CLP-S only).
    ///
    /// # Returns
    ///
    /// The distinct timestamp column names, ordered by name.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidDatasetName`] if the dataset name is invalid.
    /// * [`ClientError::DatasetNotFound`] if the dataset's column-metadata table doesn't
    ///   exist.
    /// * Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure.
    pub async fn get_timestamp_column_names(
        &self,
        dataset_name: &str,
    ) -> Result<Vec<String>, ClientError> {
        /// `MySQL` error number for `Table doesn't exist`.
        const MYSQL_TABLE_NOT_FOUND: u16 = 1146;

        if !VALID_DATASET_NAME_REGEX.is_match(dataset_name) {
            return Err(ClientError::InvalidDatasetName);
        }
        let table_name = self.table_name(Some(dataset_name), "column_metadata");
        let names: Vec<String> = sqlx::query_scalar(&format!(
            "SELECT DISTINCT name FROM `{table_name}` WHERE type IN (?, ?) ORDER BY name"
        ))
        .bind(TIMESTAMP_TYPE)
        .bind(DEPRECATED_TIMESTAMP_TYPE)
        .fetch_all(&self.sql_pool)
        .await
        .map_err(|err| {
            if let sqlx::Error::Database(db_err) = &err
                && let Some(mysql_err) =
                    db_err.try_downcast_ref::<sqlx::mysql::MySqlDatabaseError>()
                && mysql_err.number() == MYSQL_TABLE_NOT_FOUND
            {
                return ClientError::DatasetNotFound(dataset_name.to_owned());
            }
            err.into()
        })?;
        Ok(names)
    }

    /// Fetches recent compression-job metadata, with the decoded CLP IO config for each job.
    ///
    /// # Returns
    ///
    /// The compression-job metadata rows, most recent first.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::MalformedData`] if a stored `clp_config` cannot be decoded.
    /// * Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure.
    pub async fn get_compression_metadata(&self) -> Result<Vec<CompressionMetadata>, ClientError> {
        let rows = sqlx::query(
            "SELECT \
               id, status, status_msg, start_time, update_time, duration, \
               uncompressed_size, compressed_size, clp_config \
             FROM compression_jobs \
             ORDER BY id DESC \
             LIMIT ?",
        )
        .bind(COMPRESSION_METADATA_QUERY_LIMIT)
        .fetch_all(&self.sql_pool)
        .await?;
        let mut out = Vec::with_capacity(rows.len());
        for row in rows {
            let clp_io_config: serde_json::Value =
                ZstdMsgpack::deserialize(row.try_get::<Vec<u8>, _>("clp_config")?.as_slice())?;
            out.push(CompressionMetadata {
                _id: row.try_get("id")?,
                status: row.try_get("status")?,
                status_msg: row.try_get("status_msg")?,
                start_time: row
                    .try_get::<Option<DateTime<Utc>>, _>("start_time")
                    .ok()
                    .flatten()
                    .map(|dt| dt.to_rfc3339()),
                update_time: row
                    .try_get::<Option<DateTime<Utc>>, _>("update_time")
                    .ok()
                    .flatten()
                    .map(|dt| dt.to_rfc3339())
                    .unwrap_or_default(),
                duration: row.try_get("duration")?,
                uncompressed_size: row.try_get("uncompressed_size")?,
                compressed_size: row.try_get("compressed_size")?,
                clp_config: clp_io_config,
            });
        }
        Ok(out)
    }

    /// Submits a compression job to the `compression_jobs` table.
    ///
    /// The job config is encoded as msgpack and zstd-compressed before being stored.
    ///
    /// # Returns
    ///
    /// The ID of the newly created compression job on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidDatasetName`] if the dataset name is invalid.
    /// * [`ClientError::InvalidInput`] if the package is configured with S3 logs input.
    /// * Forwards [`ZstdMsgpack::serialize`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    pub async fn submit_compression_job(
        &self,
        creation: CompressionJobCreation,
    ) -> Result<CompressionJob, ClientError> {
        if let Some(dataset) = creation.dataset.as_deref()
            && !VALID_DATASET_NAME_REGEX.is_match(dataset)
        {
            return Err(ClientError::InvalidDatasetName);
        }
        let archive_output = &self.config.archive_output;
        let storage_engine = &self.config.package.storage_engine;

        let LogsInput::Fs { .. } = &self.config.logs_input else {
            return Err(ClientError::InvalidInput(
                "compression-job submission is not supported for S3 logs input".to_owned(),
            ));
        };
        let paths_to_compress: Vec<String> = creation
            .paths
            .iter()
            .map(|path| {
                format!(
                    "{CONTAINER_INPUT_LOGS_ROOT_DIR}/{}",
                    path.trim_start_matches('/')
                )
            })
            .collect();

        let mut input = serde_json::json!({
            "dataset": null,
            "path_prefix_to_remove": CONTAINER_INPUT_LOGS_ROOT_DIR,
            "paths_to_compress": paths_to_compress,
            "timestamp_key": null,
            "type": "fs",
            "unstructured": true,
        });
        let output = serde_json::json!({
            "compression_level": archive_output.compression_level,
            "target_archive_size": archive_output.target_archive_size,
            "target_dictionaries_size": archive_output.target_dictionaries_size,
            "target_encoded_file_size": archive_output.target_encoded_file_size,
            "target_segment_size": archive_output.target_segment_size,
        });

        if &StorageEngine::ClpS == storage_engine {
            input["unstructured"] = serde_json::Value::Bool(false);
            if let Some(dataset) = &creation.dataset
                && !dataset.is_empty()
            {
                input["dataset"] = serde_json::Value::String(dataset.clone());
            }
            if let Some(timestamp_key) = &creation.timestamp_key {
                input["timestamp_key"] = serde_json::Value::String(timestamp_key.clone());
            }
            if Some(true) == creation.unstructured {
                input["unstructured"] = serde_json::Value::Bool(true);
            }
        }

        let compressed =
            ZstdMsgpack::serialize(&serde_json::json!({"input": input, "output": output}))?;

        let result = sqlx::query("INSERT INTO compression_jobs (clp_config) VALUES (?)")
            .bind(compressed)
            .execute(&self.sql_pool)
            .await?;
        Ok(CompressionJob {
            job_id: i64::try_from(result.last_insert_id()).map_err(|_| {
                ClientError::InvalidInput("compression job id out of range".to_owned())
            })?,
        })
    }

    /// Lists files and directories at the specified path.
    ///
    /// # Returns
    ///
    /// The directory entries at the path, or an empty list if the path is not a directory.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::NotFound`] if the path does not exist.
    /// * [`ClientError::Io`] if the path cannot be read.
    pub async fn list_files(&self, path: String) -> Result<Vec<DirEntry>, ClientError> {
        let path_buf = PathBuf::from(&path);
        let metadata = tokio::fs::metadata(&path_buf).await.map_err(|err| {
            if err.kind() == std::io::ErrorKind::NotFound {
                ClientError::NotFound(path)
            } else {
                ClientError::Io(err)
            }
        })?;
        if !metadata.is_dir() {
            return Ok(Vec::new());
        }
        let mut entries = tokio::fs::read_dir(&path_buf).await?;
        let mut out = Vec::new();
        while let Some(entry) = entries.next_entry().await? {
            let file_type = entry.file_type().await?;
            let is_expandable = file_type.is_dir() || file_type.is_symlink();
            let name = entry.file_name().to_string_lossy().into_owned();
            let parent_path = path_buf.to_string_lossy().into_owned();
            out.push(DirEntry {
                is_expandable,
                name,
                parent_path,
            });
        }
        Ok(out)
    }

    /// Extracts a stream file containing the log event at `log_event_idx` in the stream with
    /// the given `stream_id`. If the stream has already been extracted, returns its metadata
    /// directly; otherwise submits an extraction job and waits for it to complete.
    ///
    /// # Returns
    ///
    /// The extracted stream file's metadata. Its `path` is the resolved stream-file path:
    /// a pre-signed URL when stream-files S3 storage is configured, otherwise a path
    /// relative to the webui `/streams` static mount.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidDatasetName`] if the dataset name is invalid.
    /// * [`ClientError::InvalidInput`] if `stream_id` is empty, or if the extract job fails,
    ///   is cancelled, or produces
    ///   no stream file containing the log event.
    /// * [`ClientError::Aws`] if a pre-signed URL couldn't be generated.
    /// * Forwards [`mongodb::error::Error`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    pub async fn extract_stream_file(
        &self,
        extraction: StreamFileExtraction,
    ) -> Result<StreamFileMetadata, ClientError> {
        if let Some(dataset) = extraction.dataset.as_deref()
            && !VALID_DATASET_NAME_REGEX.is_match(dataset)
        {
            return Err(ClientError::InvalidDatasetName);
        }
        if extraction.stream_id.is_empty() {
            return Err(ClientError::InvalidInput(
                "stream_id must not be empty".to_owned(),
            ));
        }
        let stream_files_collection = self
            .mongodb_client
            .database(&self.config.results_cache.db_name)
            .collection::<StreamFileMetadataDoc>(&self.config.results_cache.stream_collection_name);

        let existing = stream_files_collection
            .find_one(doc! {
                "stream_id": &extraction.stream_id,
                "begin_msg_ix": {"$lte": extraction.log_event_idx},
                "end_msg_ix": {"$gt": extraction.log_event_idx},
            })
            .await?;
        let mut metadata = if let Some(doc) = existing {
            doc.into_metadata()
        } else {
            self.submit_and_wait_extract_job(&extraction).await?;
            let doc = stream_files_collection
                .find_one(doc! {
                    "stream_id": &extraction.stream_id,
                    "begin_msg_ix": {"$lte": extraction.log_event_idx},
                    "end_msg_ix": {"$gt": extraction.log_event_idx},
                })
                .await?
                .ok_or_else(|| {
                    ClientError::InvalidInput(format!(
                        "Unable to extract stream with streamId={} at logEventIdx={}",
                        extraction.stream_id, extraction.log_event_idx
                    ))
                })?;
            doc.into_metadata()
        };

        metadata.path = match &self.config.stream_output.storage {
            StreamOutputStorage::S3 { s3_config, .. } => {
                self.generate_presigned_stream_url(s3_config, &metadata.path)
                    .await?
            }
            StreamOutputStorage::Fs { .. } => format!("/streams/{}", metadata.path),
        };
        Ok(metadata)
    }

    /// Generates a pre-signed GET URL for the stream file at `path` under the stream-output
    /// S3 key prefix.
    ///
    /// # Returns
    ///
    /// The pre-signed URL string on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::Aws`] if the stream-output S3 client was not configured, or if a
    ///   pre-signed URL couldn't be generated.
    async fn generate_presigned_stream_url(
        &self,
        s3_config: &S3Config,
        path: &str,
    ) -> Result<String, ClientError> {
        let s3_client = self
            .stream_output_s3_client
            .as_ref()
            .ok_or_else(|| ClientError::Aws {
                description: "the stream-output S3 client was not configured".to_owned(),
            })?;
        let presigning_config =
            PresigningConfig::expires_in(Duration::from_secs(PRE_SIGNED_URL_EXPIRY_TIME_SECONDS))
                .map_err(|err| ClientError::Aws {
                description: err.to_string(),
            })?;
        let request = s3_client
            .get_object()
            .bucket(s3_config.bucket.as_str())
            .key(format!("{}{path}", s3_config.key_prefix))
            .presigned(presigning_config)
            .await?;
        Ok(request.uri().to_owned())
    }

    /// Submits a stream extraction job to the `query_jobs` table and polls its status until
    /// it finishes.
    ///
    /// # Returns
    ///
    /// `Ok(())` once the extract job succeeds.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::SearchJobNotFound`] if the job disappears from the database.
    /// * [`ClientError::InvalidInput`] if the job fails, is killed, or is cancelled.
    /// * [`ClientError::MalformedData`] if the job reports an unrecognized status.
    /// * Forwards [`rmp_serde::to_vec_named`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    async fn submit_and_wait_extract_job(
        &self,
        extraction: &StreamFileExtraction,
    ) -> Result<(), ClientError> {
        let target_uncompressed_size = self.config.stream_output.target_uncompressed_size;
        let job_config = match extraction.extract_job_type {
            ExtractJobType::ExtractIr => serde_json::json!({
                "file_split_id": null,
                "msg_ix": extraction.log_event_idx,
                "orig_file_id": extraction.stream_id,
                "target_uncompressed_size": target_uncompressed_size,
            }),
            ExtractJobType::ExtractJson => serde_json::json!({
                "dataset": extraction.dataset,
                "archive_id": extraction.stream_id,
                "target_chunk_size": target_uncompressed_size,
            }),
        };
        let encoded = rmp_serde::to_vec_named(&job_config)?;
        let job_type_i32: i32 = extraction.extract_job_type.into();
        let result = sqlx::query("INSERT INTO query_jobs (job_config, type) VALUES (?, ?)")
            .bind(encoded)
            .bind(job_type_i32)
            .execute(&self.sql_pool)
            .await?;
        let job_id = result.last_insert_id();

        let mut delay_ms = 100u64;
        loop {
            let row = sqlx::query("SELECT status FROM query_jobs WHERE id = ?")
                .bind(job_id)
                .fetch_optional(&self.sql_pool)
                .await?;
            let Some(row) = row else {
                return Err(ClientError::SearchJobNotFound(job_id));
            };
            let status: i32 = row.try_get("status")?;
            match QueryJobStatus::try_from(status)? {
                QueryJobStatus::Succeeded => break,
                QueryJobStatus::Pending | QueryJobStatus::Running | QueryJobStatus::Cancelling => {
                    tokio::time::sleep(Duration::from_millis(delay_ms)).await;
                    delay_ms = std::cmp::min(delay_ms.saturating_mul(2), 5000);
                }
                QueryJobStatus::Cancelled => {
                    return Err(ClientError::InvalidInput(format!(
                        "Extract job {job_id} was cancelled"
                    )));
                }
                QueryJobStatus::Failed | QueryJobStatus::Killed => {
                    return Err(ClientError::InvalidInput(format!(
                        "Extract job {job_id} exited with unexpected status={status}"
                    )));
                }
            }
        }
        Ok(())
    }
}

/// Mirror of `CONTAINER_INPUT_LOGS_ROOT_DIR` in `clp_package_utils.general`.
const CONTAINER_INPUT_LOGS_ROOT_DIR: &str = "/mnt/logs";

/// Internal document shape for the stream-files `MongoDB` collection.
#[derive(Debug, Deserialize)]
struct StreamFileMetadataDoc {
    begin_msg_ix: i64,
    end_msg_ix: i64,
    is_last_chunk: bool,
    path: String,
    stream_id: String,
}

impl StreamFileMetadataDoc {
    /// Converts the document into the public [`StreamFileMetadata`] shape.
    fn into_metadata(self) -> StreamFileMetadata {
        StreamFileMetadata {
            begin_msg_ix: self.begin_msg_ix,
            end_msg_ix: self.end_msg_ix,
            is_last_chunk: self.is_last_chunk,
            path: self.path,
            stream_id: self.stream_id,
        }
    }
}

/// Expiry time in seconds for pre-signed stream-file URLs.
const PRE_SIGNED_URL_EXPIRY_TIME_SECONDS: u64 = 3600;
