//! A client dedicated to metadata, compression-job, and stream-file operations.
//!
//! This is intentionally separate from [`crate::client::Client`] (which handles search
//! query orchestration) so that the metadata access surface stays independent and easy to
//! reason about.

use std::path::Component;
use std::path::Path;
use std::path::PathBuf;
use std::time::Duration;

use aws_sdk_s3::presigning::PresigningConfig;
use chrono::DateTime;
use chrono::Utc;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package::config::ArchiveOutput;
use clp_rust_utils::clp_config::package::config::CONTAINER_INPUT_LOGS_ROOT_DIR;
use clp_rust_utils::clp_config::package::config::Config;
use clp_rust_utils::clp_config::package::config::Database;
use clp_rust_utils::clp_config::package::config::LogsInput;
use clp_rust_utils::clp_config::package::config::MetadataTableScope;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
use clp_rust_utils::clp_config::package::credentials::Credentials;
use clp_rust_utils::database::mysql::create_clp_db_mysql_pool;
use clp_rust_utils::dataset::is_valid_dataset_name;
use clp_rust_utils::dataset::resolve_dataset_name;
use clp_rust_utils::job_config::COMPRESSION_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QUERY_TASKS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::QueryJobType;
use clp_rust_utils::serde::ZstdMsgpack;
use mongodb::bson::doc;
use serde::Deserialize;
use serde::Serialize;
use sqlx::Row;
use utoipa::ToSchema;

use crate::error::ClientError;

/// Schema mirror of `NodeType::DeprecatedDateString` and `NodeType::Timestamp` in
/// `components/core/src/clp_s/SchemaTree.hpp`.
const DEPRECATED_TIMESTAMP_TYPE: i8 = 8;
const TIMESTAMP_TYPE: i8 = 14;

/// `MySQL` error number for `Table doesn't exist`.
const MYSQL_TABLE_NOT_FOUND: u16 = 1146;

/// Maximum number of compression-metadata rows to return.
const COMPRESSION_METADATA_QUERY_LIMIT: i64 = 1000;

/// Request body for submitting a compression job.
#[derive(Clone, Debug, Deserialize, Serialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct CompressionJobCreation {
    /// User-facing absolute paths, relative to the configured logs-input root.
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
    pub job_id: i64,
}

/// A row of compression metadata, with the decoded CLP IO config.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct CompressionMetadata {
    /// The compression job's ID. Named `_id` to match the webui's existing JSON contract.
    #[allow(clippy::pub_underscore_fields)]
    pub _id: i64,
    pub status: i32,
    pub status_msg: String,
    pub start_time: Option<String>,
    pub update_time: String,
    pub duration: Option<f64>,
    pub uncompressed_size: i64,
    pub compressed_size: i64,
    /// Decoded CLP IO config (as a JSON value) since the stored config is a zstd-compressed
    /// msgpack blob.
    pub clp_config: serde_json::Value,
}

/// Aggregated space-savings statistics.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct SpaceSavings {
    pub total_uncompressed_size: i64,
    pub total_compressed_size: i64,
}

/// Ingestion details statistics.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct IngestionDetails {
    pub begin_timestamp: Option<i64>,
    pub end_timestamp: Option<i64>,
    pub num_files: Option<i64>,
    pub num_messages: Option<i64>,
}

/// Query-speed statistics for a search job.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct QuerySpeed {
    pub bytes: Option<f64>,
    pub duration: Option<f64>,
}

/// Earliest and latest log entry timestamps across the selected datasets.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct TimeRange {
    pub begin_timestamp: Option<i64>,
    pub end_timestamp: Option<i64>,
}

/// A directory entry returned by the file-listing endpoint.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct DirEntry {
    pub is_expandable: bool,
    pub name: String,
    pub parent_path: String,
}

/// Extracted stream-file metadata returned by the stream-files extract endpoint.
#[derive(Clone, Debug, Serialize, Deserialize, ToSchema)]
pub struct StreamFileMetadata {
    pub begin_msg_ix: i64,
    pub end_msg_ix: i64,
    pub is_last_chunk: bool,
    pub path: String,
    pub stream_id: String,
}

/// Request body for the stream-files extract endpoint.
#[derive(Clone, Debug, Deserialize, Serialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamFileExtraction {
    /// Dataset the stream belongs to (CLP-S only; `null` for the CLP storage engine).
    #[serde(default)]
    pub dataset: Option<String>,
    pub extract_job_type: QueryJobType,
    pub log_event_idx: i64,
    pub stream_id: String,
}

/// A dedicated client for metadata, compression-job, and stream-file operations.
///
/// Unlike [`crate::client::Client`], this client does not handle search query orchestration;
/// it only reads metadata and submits compression/extract jobs.
#[derive(Clone)]
pub struct MetadataClient {
    mongodb_client: mongodb::Client,
    sql_pool: sqlx::Pool<sqlx::MySql>,
    config: Config,
}

impl MetadataClient {
    /// Factory method to create a new [`MetadataClient`] with active connections to both
    /// `MySQL` and `MongoDB`.
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

        Ok(Self {
            config: config.clone(),
            mongodb_client: mongo_client,
            sql_pool,
        })
    }

    const fn database(&self) -> &Database {
        &self.config.database
    }

    fn storage_engine(&self) -> StorageEngine {
        self.config.package.storage_engine.clone()
    }

    const fn archive_output(&self) -> &ArchiveOutput {
        &self.config.archive_output
    }

    /// Validates that `dataset` (when set) is a usable dataset name — see
    /// [`is_valid_dataset_name`].
    fn validate_dataset(dataset: Option<&str>) -> Result<(), ClientError> {
        if let Some(name) = dataset
            && !is_valid_dataset_name(name)
        {
            return Err(ClientError::InvalidDatasetName);
        }
        Ok(())
    }

    /// # Returns
    ///
    /// The table scope for `dataset` under the configured storage engine: [`Global`] for CLP,
    /// which keeps a single dataset-less table set, and the named (or default) dataset for CLP-S.
    ///
    /// [`Global`]: MetadataTableScope::Global
    fn table_scope<'a>(&self, dataset: Option<&'a str>) -> MetadataTableScope<'a> {
        match self.storage_engine() {
            StorageEngine::Clp => MetadataTableScope::Global,
            StorageEngine::ClpS => MetadataTableScope::Dataset(resolve_dataset_name(dataset)),
        }
    }

    /// # Returns
    ///
    /// The archives table name for `dataset` under the configured storage engine.
    fn archives_table(&self, dataset: Option<&str>) -> String {
        self.database()
            .archives_table_name(self.table_scope(dataset))
    }

    /// # Returns
    ///
    /// The files table name for `dataset` under the configured storage engine.
    fn files_table(&self, dataset: Option<&str>) -> String {
        self.database().files_table_name(self.table_scope(dataset))
    }

    /// Fetches all dataset names from the datasets table, ordered by name.
    ///
    /// # Errors
    ///
    /// Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure.
    pub async fn get_dataset_names(&self) -> Result<Vec<String>, ClientError> {
        let table = self.database().datasets_table_name();
        let names: Vec<String> =
            sqlx::query_scalar(&format!("SELECT name FROM `{table}` ORDER BY name"))
                .fetch_all(&self.sql_pool)
                .await?;
        Ok(names)
    }

    /// Fetches the earliest and latest log entry timestamps across the given datasets.
    ///
    /// For the CLP storage engine, `datasets` is ignored and the single `clp_archives` table
    /// is queried. For CLP-S, the union of the per-dataset archives tables is queried; an
    /// empty `datasets` list returns a `None` time range.
    ///
    /// # Errors
    ///
    /// Returns [`ClientError::InvalidDatasetName`] if any dataset name is invalid.
    /// Forwards [`sqlx::query::Query::fetch_optional`]'s return values on failure.
    pub async fn get_time_range(&self, datasets: &[String]) -> Result<TimeRange, ClientError> {
        for d in datasets {
            Self::validate_dataset(Some(d.as_str()))?;
        }
        match self.storage_engine() {
            StorageEngine::Clp => {
                let table = self.archives_table(None);
                let row = sqlx::query(&format!(
                    "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS \
                     end_timestamp FROM `{table}`"
                ))
                .fetch_optional(&self.sql_pool)
                .await?;
                Self::row_to_time_range(row)
            }
            StorageEngine::ClpS => {
                if datasets.is_empty() {
                    return Ok(TimeRange {
                        begin_timestamp: None,
                        end_timestamp: None,
                    });
                }
                let union = datasets
                    .iter()
                    .map(|d| {
                        let table = self.archives_table(Some(d));
                        format!(
                            "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) \
                             AS end_timestamp FROM `{table}`"
                        )
                    })
                    .collect::<Vec<_>>()
                    .join("\nUNION ALL\n");
                let sql = format!(
                    "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS \
                     end_timestamp FROM ({union}) AS combined"
                );
                let row = sqlx::query(&sql).fetch_optional(&self.sql_pool).await?;
                Self::row_to_time_range(row)
            }
        }
    }

    fn row_to_time_range(row: Option<sqlx::mysql::MySqlRow>) -> Result<TimeRange, ClientError> {
        let Some(row) = row else {
            return Ok(TimeRange {
                begin_timestamp: None,
                end_timestamp: None,
            });
        };
        Ok(TimeRange {
            begin_timestamp: row.try_get("begin_timestamp")?,
            end_timestamp: row.try_get("end_timestamp")?,
        })
    }

    /// Fetches aggregated space-savings statistics (total uncompressed and compressed sizes)
    /// across the given datasets.
    ///
    /// For CLP, `datasets` is ignored. For CLP-S, an empty `datasets` list returns zeros.
    ///
    /// # Errors
    ///
    /// Returns [`ClientError::InvalidDatasetName`] if any dataset name is invalid.
    /// Forwards [`sqlx::query::Query::fetch_optional`]'s return values on failure.
    pub async fn get_space_savings(
        &self,
        datasets: &[String],
    ) -> Result<SpaceSavings, ClientError> {
        for d in datasets {
            Self::validate_dataset(Some(d.as_str()))?;
        }
        let sql = match self.storage_engine() {
            StorageEngine::Clp => {
                let table = self.archives_table(None);
                format!(
                    "SELECT CAST(COALESCE(SUM(uncompressed_size), 0) AS SIGNED) AS \
                     total_uncompressed_size, CAST(COALESCE(SUM(size), 0) AS SIGNED) AS \
                     total_compressed_size FROM `{table}`"
                )
            }
            StorageEngine::ClpS => {
                if datasets.is_empty() {
                    return Ok(SpaceSavings {
                        total_uncompressed_size: 0,
                        total_compressed_size: 0,
                    });
                }
                let union = datasets
                    .iter()
                    .map(|d| {
                        let table = self.archives_table(Some(d));
                        format!("SELECT uncompressed_size, size FROM `{table}`")
                    })
                    .collect::<Vec<_>>()
                    .join("\nUNION ALL\n");
                format!(
                    "SELECT CAST(COALESCE(SUM(uncompressed_size), 0) AS SIGNED) AS \
                     total_uncompressed_size, CAST(COALESCE(SUM(size), 0) AS SIGNED) AS \
                     total_compressed_size FROM ({union}) AS archives_combined"
                )
            }
        };
        let row = sqlx::query(&sql).fetch_optional(&self.sql_pool).await?;
        let Some(row) = row else {
            return Ok(SpaceSavings {
                total_uncompressed_size: 0,
                total_compressed_size: 0,
            });
        };
        Ok(SpaceSavings {
            total_uncompressed_size: row.try_get("total_uncompressed_size")?,
            total_compressed_size: row.try_get("total_compressed_size")?,
        })
    }

    /// Fetches ingestion details (timestamp range, file count, message count) across the
    /// given datasets.
    ///
    /// For CLP, `datasets` is ignored. For CLP-S, an empty `datasets` list returns all-null
    /// counts.
    ///
    /// # Errors
    ///
    /// Returns [`ClientError::InvalidDatasetName`] if any dataset name is invalid.
    /// Forwards [`sqlx::query::Query::fetch_optional`]'s return values on failure.
    pub async fn get_ingestion_details(
        &self,
        datasets: &[String],
    ) -> Result<IngestionDetails, ClientError> {
        for d in datasets {
            Self::validate_dataset(Some(d.as_str()))?;
        }
        let sql = match self.storage_engine() {
            StorageEngine::Clp => {
                let archives = self.archives_table(None);
                let files = self.files_table(None);
                build_ingestion_details_query(
                    &format!(
                        "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS \
                         end_timestamp FROM `{archives}`"
                    ),
                    &format!(
                        "SELECT COUNT(DISTINCT orig_file_id) AS num_files, \
                         CAST(COALESCE(SUM(num_messages), 0) AS SIGNED) AS num_messages FROM \
                         `{files}`"
                    ),
                )
            }
            StorageEngine::ClpS => {
                if datasets.is_empty() {
                    return Ok(IngestionDetails {
                        begin_timestamp: None,
                        end_timestamp: None,
                        num_files: Some(0),
                        num_messages: Some(0),
                    });
                }
                let archives_union = datasets
                    .iter()
                    .map(|d| {
                        let table = self.archives_table(Some(d));
                        format!(
                            "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) \
                             AS end_timestamp FROM `{table}`"
                        )
                    })
                    .collect::<Vec<_>>()
                    .join("\nUNION ALL\n");
                let files_union = datasets
                    .iter()
                    .map(|d| {
                        let table = self.files_table(Some(d));
                        format!(
                            "SELECT COUNT(DISTINCT orig_file_id) AS num_files, \
                             CAST(COALESCE(SUM(num_messages), 0) AS SIGNED) AS num_messages FROM \
                             `{table}`"
                        )
                    })
                    .collect::<Vec<_>>()
                    .join("\nUNION ALL\n");
                build_ingestion_details_query(
                    &format!(
                        "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS \
                         end_timestamp FROM ({archives_union}) AS archives_combined"
                    ),
                    &format!(
                        "SELECT CAST(SUM(num_files) AS SIGNED) AS num_files, \
                         CAST(SUM(num_messages) AS SIGNED) AS num_messages FROM ({files_union}) \
                         AS files_combined"
                    ),
                )
            }
        };
        let row = sqlx::query(&sql).fetch_optional(&self.sql_pool).await?;
        let Some(row) = row else {
            return Ok(IngestionDetails {
                begin_timestamp: None,
                end_timestamp: None,
                num_files: None,
                num_messages: None,
            });
        };
        Ok(IngestionDetails {
            begin_timestamp: row.try_get("begin_timestamp")?,
            end_timestamp: row.try_get("end_timestamp")?,
            num_files: row.try_get("num_files")?,
            num_messages: row.try_get("num_messages")?,
        })
    }

    /// Fetches the query speed (total uncompressed bytes scanned and job duration) for a
    /// search job across the given datasets.
    ///
    /// # Errors
    ///
    /// Returns [`ClientError::InvalidDatasetName`] if any dataset name is invalid.
    /// Forwards [`sqlx::query::Query::fetch_optional`]'s return values on failure.
    pub async fn get_query_speed(
        &self,
        datasets: &[String],
        search_job_id: i64,
    ) -> Result<QuerySpeed, ClientError> {
        for d in datasets {
            Self::validate_dataset(Some(d.as_str()))?;
        }
        let archives_subquery = match self.storage_engine() {
            StorageEngine::Clp => {
                let table = self.archives_table(None);
                format!("SELECT id, uncompressed_size FROM `{table}`")
            }
            StorageEngine::ClpS => {
                if datasets.is_empty() {
                    return Ok(QuerySpeed {
                        bytes: None,
                        duration: None,
                    });
                }
                datasets
                    .iter()
                    .map(|d| {
                        let table = self.archives_table(Some(d));
                        format!("SELECT id, uncompressed_size FROM `{table}`")
                    })
                    .collect::<Vec<_>>()
                    .join(" UNION ALL ")
            }
        };
        let sql = format!(
            "WITH qt AS ( SELECT job_id, archive_id FROM {QUERY_TASKS_TABLE_NAME} WHERE \
             archive_id IS NOT NULL AND job_id = ? ), totals AS ( SELECT qt.job_id, \
             SUM(ca.uncompressed_size) AS total_uncompressed_bytes FROM qt JOIN \
             ({archives_subquery}) ca ON qt.archive_id = ca.id ) SELECT \
             CAST(totals.total_uncompressed_bytes AS DOUBLE) AS bytes, qj.duration AS duration \
             FROM {QUERY_JOBS_TABLE_NAME} qj JOIN totals ON totals.job_id = qj.id"
        );
        let row = sqlx::query(&sql)
            .bind(search_job_id)
            .fetch_optional(&self.sql_pool)
            .await?;
        let Some(row) = row else {
            return Ok(QuerySpeed {
                bytes: None,
                duration: None,
            });
        };
        Ok(QuerySpeed {
            bytes: row.try_get("bytes")?,
            duration: row.try_get("duration")?,
        })
    }

    /// Fetches the timestamp column names for a given dataset (CLP-S only).
    ///
    /// # Errors
    ///
    /// Returns [`ClientError::InvalidDatasetName`] if the dataset name is invalid.
    /// Returns [`ClientError::DatasetNotFound`] if the dataset's column-metadata table
    /// doesn't exist.
    /// Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure.
    pub async fn get_timestamp_column_names(
        &self,
        dataset_name: &str,
    ) -> Result<Vec<String>, ClientError> {
        if !is_valid_dataset_name(dataset_name) {
            return Err(ClientError::InvalidDatasetName);
        }
        let table_name = self.database().column_metadata_table_name(dataset_name);
        let names: Vec<String> =
            sqlx::query_scalar(&build_timestamp_column_names_query(&table_name))
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

    /// Fetches recent compression-job metadata (most recent first), with the decoded CLP IO
    /// config for each job.
    ///
    /// # Errors
    ///
    /// Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure.
    pub async fn get_compression_metadata(&self) -> Result<Vec<CompressionMetadata>, ClientError> {
        let rows = sqlx::query(&format!(
            "SELECT id, status, status_msg, start_time, update_time, duration, uncompressed_size, \
             compressed_size, clp_config FROM {COMPRESSION_JOBS_TABLE_NAME} ORDER BY id DESC \
             LIMIT ?"
        ))
        .bind(COMPRESSION_METADATA_QUERY_LIMIT)
        .fetch_all(&self.sql_pool)
        .await?;
        let mut out = Vec::with_capacity(rows.len());
        for row in rows {
            let clp_config_blob: Vec<u8> = row.try_get("clp_config")?;
            let clp_config = decode_clp_config(&clp_config_blob)?;
            let start_time: Option<DateTime<Utc>> = row.try_get("start_time")?;
            let update_time: Option<DateTime<Utc>> = row.try_get("update_time")?;
            out.push(CompressionMetadata {
                _id: row.try_get("id")?,
                status: row.try_get("status")?,
                status_msg: row.try_get("status_msg")?,
                start_time: start_time.map(|dt| dt.to_rfc3339()),
                update_time: update_time.map_or_else(String::new, |dt| dt.to_rfc3339()),
                duration: row.try_get("duration")?,
                uncompressed_size: row.try_get("uncompressed_size")?,
                compressed_size: row.try_get("compressed_size")?,
                clp_config,
            });
        }
        Ok(out)
    }

    /// Submits a compression job to the `compression_jobs` table.
    ///
    /// The job config is encoded as msgpack and zstd-compressed before being stored, mirroring
    /// the webui server's `CompressionJobDbManager`.
    ///
    /// # Errors
    ///
    /// Returns [`ClientError::InvalidDatasetName`] if the dataset name is invalid.
    /// Forwards [`ZstdMsgpack::serialize`]'s return values on failure.
    /// Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    pub async fn submit_compression_job(
        &self,
        creation: CompressionJobCreation,
    ) -> Result<CompressionJob, ClientError> {
        validate_compression_job_creation(&creation)?;
        let archive_output = self.archive_output();
        let storage_engine = self.storage_engine();
        let LogsInput::Fs { config: logs_input } = &self.config.logs_input else {
            return Err(ClientError::InvalidInput(
                "Filesystem compression is unavailable when logs_input is not filesystem-backed"
                    .to_owned(),
            ));
        };
        let paths_to_compress =
            resolve_compression_paths(Path::new(&logs_input.directory), &creation.paths).await?;

        let job_config = build_compression_job_config(
            &storage_engine,
            archive_output,
            &paths_to_compress,
            &creation,
        );
        let compressed = ZstdMsgpack::serialize(&job_config)?;

        let result = sqlx::query(&format!(
            "INSERT INTO {COMPRESSION_JOBS_TABLE_NAME} (clp_config) VALUES (?)"
        ))
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
    /// Paths outside the configured logs-input root are rejected before the requested path is
    /// touched, so they cannot be used to probe for the existence of arbitrary paths.
    ///
    /// # Errors
    ///
    /// Returns [`ClientError::InvalidInput`] if filesystem input is not configured, if the path
    /// is relative, if it contains a parent-directory component, or if it is outside the
    /// configured input root.
    /// Returns [`ClientError::NotFound`] if a path inside the root doesn't exist.
    /// Returns [`ClientError::Io`] if the path cannot be read.
    pub async fn list_files(&self, path: String) -> Result<Vec<DirEntry>, ClientError> {
        let path_buf = PathBuf::from(&path);
        if !path_buf.is_absolute() {
            return Err(ClientError::InvalidInput(
                "File-listing paths must be absolute".to_owned(),
            ));
        }
        let LogsInput::Fs { config: logs_input } = &self.config.logs_input else {
            return Err(ClientError::InvalidInput(
                "File listing is unavailable when logs_input is not filesystem-backed".to_owned(),
            ));
        };
        let configured_root = Path::new(&logs_input.directory);
        let canonical_root = tokio::fs::canonicalize(configured_root).await?;
        let normalized = normalize_listing_path(&path_buf, &path)?;
        if !is_under_logs_input_root(&normalized, configured_root, &canonical_root) {
            return Err(ClientError::InvalidInput(format!(
                "Path '{path}' is outside the configured logs-input directory"
            )));
        }

        let canonical_path = canonicalize_listing_path(&normalized, &path).await?;
        if !canonical_path.starts_with(&canonical_root) {
            return Err(ClientError::InvalidInput(format!(
                "Path '{path}' is outside the configured logs-input directory"
            )));
        }

        let metadata = tokio::fs::metadata(&canonical_path).await?;
        if !metadata.is_dir() {
            return Ok(Vec::new());
        }
        let mut entries = tokio::fs::read_dir(&canonical_path).await?;
        let mut out = Vec::new();
        while let Some(entry) = entries.next_entry().await? {
            let file_type = entry.file_type().await?;
            let is_expandable = if file_type.is_dir() {
                true
            } else if file_type.is_symlink() {
                let target = tokio::fs::canonicalize(entry.path()).await;
                match target {
                    Ok(target) if target.starts_with(&canonical_root) => {
                        tokio::fs::metadata(target).await?.is_dir()
                    }
                    Ok(_) | Err(_) => false,
                }
            } else {
                false
            };
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
    /// The returned `path` is the resolved stream-file path. When stream-files S3 storage is
    /// configured, this is a pre-signed URL; otherwise it is a path relative to the webui
    /// `/streams` static mount.
    ///
    /// # Errors
    ///
    /// Returns [`ClientError::InvalidDatasetName`] if the dataset name is invalid.
    /// Returns [`ClientError::InvalidInput`] if the extract job type is invalid.
    /// Forwards [`mongodb::error::Error`]'s return values on failure.
    /// Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    pub async fn extract_stream_file(
        &self,
        extraction: StreamFileExtraction,
    ) -> Result<StreamFileMetadata, ClientError> {
        Self::validate_dataset(extraction.dataset.as_deref())?;
        if QueryJobType::SearchOrAggregation == extraction.extract_job_type {
            return Err(ClientError::InvalidInput(
                "SearchOrAggregation is not a stream-extraction job type".to_owned(),
            ));
        }
        let stream_files_collection = self
            .mongodb_client
            .database(&self.config.results_cache.db_name)
            .collection::<StreamFileMetadataDoc>(&self.config.results_cache.stream_collection_name);

        // Try to find an already-extracted stream file.
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

        metadata.path = self.resolve_stream_file_path(&metadata.path).await?;
        Ok(metadata)
    }

    async fn resolve_stream_file_path(&self, path: &str) -> Result<String, ClientError> {
        let StreamOutputStorage::S3 { s3_config, .. } = &self.config.stream_output.storage else {
            return Ok(format!("/streams/{path}"));
        };
        if s3_config.region_code.is_none() && s3_config.endpoint_url.is_none() {
            return Err(ClientError::Aws {
                description: "a region code must be given when using the default AWS S3 endpoint"
                    .to_owned(),
            });
        }

        let region = s3_config
            .region_code
            .as_ref()
            .map_or(AWS_DEFAULT_REGION, non_empty_string::NonEmptyString::as_str);
        let client = clp_rust_utils::s3::create_new_client(
            region,
            s3_config.endpoint_url.as_ref(),
            &s3_config.aws_authentication,
        )
        .await;
        let key = format!("{}{path}", s3_config.key_prefix);
        let presigning_config =
            PresigningConfig::expires_in(Duration::from_secs(3600)).map_err(|error| {
                ClientError::Aws {
                    description: error.to_string(),
                }
            })?;
        let request = client
            .get_object()
            .bucket(s3_config.bucket.as_str())
            .key(key)
            .presigned(presigning_config)
            .await
            .map_err(|error| ClientError::Aws {
                description: error.to_string(),
            })?;
        Ok(request.uri().to_string())
    }

    /// Submits an extract job to the `query_jobs` table and waits for it to complete.
    async fn submit_and_wait_extract_job(
        &self,
        extraction: &StreamFileExtraction,
    ) -> Result<(), ClientError> {
        let target_uncompressed_size = self.config.stream_output.target_uncompressed_size;
        let job_config = match extraction.extract_job_type {
            QueryJobType::ExtractIr => serde_json::json!({
                "file_split_id": null,
                "msg_ix": extraction.log_event_idx,
                "orig_file_id": extraction.stream_id,
                "target_uncompressed_size": target_uncompressed_size,
            }),
            QueryJobType::ExtractJson => serde_json::json!({
                "dataset": extraction.dataset,
                "archive_id": extraction.stream_id,
                "target_chunk_size": target_uncompressed_size,
            }),
            QueryJobType::SearchOrAggregation => {
                return Err(ClientError::InvalidInput(
                    "SearchOrAggregation is not a stream-extraction job type".to_owned(),
                ));
            }
        };
        let encoded = rmp_serde::to_vec_named(&job_config)?;
        let job_type_i32: i32 = extraction.extract_job_type.into();
        let result = sqlx::query(&format!(
            "INSERT INTO {QUERY_JOBS_TABLE_NAME} (job_config, type) VALUES (?, ?)"
        ))
        .bind(encoded)
        .bind(job_type_i32)
        .execute(&self.sql_pool)
        .await?;
        let job_id = result.last_insert_id();

        // Poll for completion.
        let timeout_secs = self
            .config
            .api_server
            .as_ref()
            .ok_or(ClientError::ConfigIsNone)?
            .stream_file_extraction_timeout_secs;
        let deadline = tokio::time::Instant::now() + Duration::from_secs(timeout_secs);
        let mut delay_ms = 100u64;
        loop {
            if tokio::time::Instant::now() >= deadline {
                return Err(ClientError::Timeout(format!(
                    "stream-extraction job {job_id} did not finish within {timeout_secs} seconds"
                )));
            }
            let row = sqlx::query(&format!(
                "SELECT status FROM {QUERY_JOBS_TABLE_NAME} WHERE id = ?"
            ))
            .bind(job_id)
            .fetch_optional(&self.sql_pool)
            .await?;
            let Some(row) = row else {
                return Err(ClientError::SearchJobNotFound(job_id));
            };
            let raw_status: i32 = row.try_get("status")?;
            let status = QueryJobStatus::try_from(raw_status)?;
            match status {
                QueryJobStatus::Succeeded => break,
                QueryJobStatus::Failed | QueryJobStatus::Killed => {
                    return Err(ClientError::InvalidInput(format!(
                        "Extract job {job_id} exited with status={status:?}"
                    )));
                }
                QueryJobStatus::Cancelled => {
                    return Err(ClientError::InvalidInput(format!(
                        "Extract job {job_id} was cancelled"
                    )));
                }
                QueryJobStatus::Pending | QueryJobStatus::Running | QueryJobStatus::Cancelling => {
                    tokio::time::sleep(tokio::time::Duration::from_millis(delay_ms)).await;
                    delay_ms = std::cmp::min(delay_ms.saturating_mul(2), 5000);
                }
            }
        }
        Ok(())
    }
}

async fn canonicalize_listing_path(
    path: &Path,
    display_path: &str,
) -> Result<PathBuf, ClientError> {
    tokio::fs::canonicalize(path).await.map_err(|error| {
        if error.kind() == std::io::ErrorKind::NotFound {
            ClientError::NotFound(display_path.to_owned())
        } else {
            ClientError::Io(error)
        }
    })
}

/// Normalizes an absolute listing path lexically, rejecting any parent-directory component.
///
/// This runs before the requested path is touched so that a path outside the configured logs-input
/// root is rejected identically whether or not it exists — otherwise the 400-vs-404 split turns the
/// endpoint into an existence oracle for arbitrary paths in the container.
///
/// # Errors
///
/// Returns [`ClientError::InvalidInput`] if `path` contains a `..` or prefix component.
fn normalize_listing_path(path: &Path, display_path: &str) -> Result<PathBuf, ClientError> {
    let mut normalized = PathBuf::new();
    for component in path.components() {
        match component {
            Component::RootDir => normalized.push(Component::RootDir.as_os_str()),
            Component::CurDir => {}
            Component::Normal(segment) => normalized.push(segment),
            Component::ParentDir | Component::Prefix(_) => {
                return Err(ClientError::InvalidInput(format!(
                    "Path '{display_path}' contains an invalid component"
                )));
            }
        }
    }
    Ok(normalized)
}

/// # Returns
///
/// Whether `normalized` sits under the logs-input root.
///
/// Both the configured directory and its canonical form are accepted: `/os/ls` receives paths the
/// Web UI built from the *configured* `logs_input.directory`, which differs from the canonical
/// form when any component of it is a symlink.
fn is_under_logs_input_root(
    normalized: &Path,
    configured_root: &Path,
    canonical_root: &Path,
) -> bool {
    normalized.starts_with(configured_root) || normalized.starts_with(canonical_root)
}

async fn resolve_compression_paths(
    root: &Path,
    requested_paths: &[String],
) -> Result<Vec<String>, ClientError> {
    let canonical_root = tokio::fs::canonicalize(root).await?;
    let mut resolved_paths = Vec::with_capacity(requested_paths.len());

    for requested_path in requested_paths {
        let path = Path::new(requested_path);
        if !path.is_absolute() {
            return Err(ClientError::InvalidInput(format!(
                "Compression path '{requested_path}' must be absolute"
            )));
        }

        let mut relative_path = PathBuf::new();
        for component in path.components() {
            match component {
                Component::RootDir | Component::CurDir => {}
                Component::Normal(segment) => relative_path.push(segment),
                Component::ParentDir | Component::Prefix(_) => {
                    return Err(ClientError::InvalidInput(format!(
                        "Compression path '{requested_path}' contains an invalid component"
                    )));
                }
            }
        }

        let canonical_path =
            canonicalize_listing_path(&canonical_root.join(relative_path), requested_path).await?;
        if !canonical_path.starts_with(&canonical_root) {
            return Err(ClientError::InvalidInput(format!(
                "Compression path '{requested_path}' resolves outside the configured logs-input \
                 directory"
            )));
        }
        resolved_paths.push(canonical_path.to_string_lossy().into_owned());
    }

    Ok(resolved_paths)
}

/// Builds the query that fetches a dataset's timestamp column names.
///
/// `DISTINCT` is required because the column-metadata table's primary key is `(name, type)`, so a
/// column recorded as both [`DEPRECATED_TIMESTAMP_TYPE`] and [`TIMESTAMP_TYPE`] would otherwise be
/// returned twice. `ORDER BY` keeps the Web UI's default timestamp-key selection deterministic.
fn build_timestamp_column_names_query(table_name: &str) -> String {
    format!("SELECT DISTINCT name FROM `{table_name}` WHERE type IN (?, ?) ORDER BY name")
}

/// Combines an archives aggregate and a files aggregate into the single row the
/// ingestion-details endpoint returns.
///
/// `archives_source` must be a sub-query producing exactly one row with `begin_timestamp` and
/// `end_timestamp` columns; `files_source` must produce exactly one row with `num_files` and
/// `num_messages`. Cross-joining two single-row sources keeps each underlying table referenced
/// exactly once — referencing them from separate scalar sub-queries instead makes `MySQL`
/// evaluate each source once per referencing column.
fn build_ingestion_details_query(archives_source: &str, files_source: &str) -> String {
    format!(
        "SELECT a.begin_timestamp AS begin_timestamp, a.end_timestamp AS end_timestamp, \
         f.num_files AS num_files, f.num_messages AS num_messages FROM ({archives_source}) AS a, \
         ({files_source}) AS f"
    )
}

/// Validates a compression-job request before any I/O, mirroring the checks the Web UI's
/// now-removed `POST /api/compress` route ran against `CompressionJobCreationSchema`.
///
/// # Errors
///
/// Returns [`ClientError::InvalidDatasetName`] if `dataset` is set but not a usable dataset name.
/// Returns [`ClientError::InvalidInput`] if `paths` is empty.
fn validate_compression_job_creation(creation: &CompressionJobCreation) -> Result<(), ClientError> {
    MetadataClient::validate_dataset(creation.dataset.as_deref())?;
    if creation.paths.is_empty() {
        return Err(ClientError::InvalidInput(
            "A compression job must specify at least one path".to_owned(),
        ));
    }
    Ok(())
}

/// Builds the `ClpIoConfig` submitted to the `compression_jobs` table.
///
/// Field names mirror `job_orchestration.scheduler.job_config`. `path_prefix_to_remove` is always
/// [`CONTAINER_INPUT_LOGS_ROOT_DIR`] so that the original paths recorded for Web UI jobs match
/// those recorded by the CLI (`clp_package_utils.scripts.native.compress`).
fn build_compression_job_config(
    storage_engine: &StorageEngine,
    archive_output: &ArchiveOutput,
    paths_to_compress: &[String],
    creation: &CompressionJobCreation,
) -> serde_json::Value {
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

    serde_json::json!({"input": input, "output": output})
}

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

/// Decodes a zstd-compressed msgpack CLP IO config blob into a JSON value.
fn decode_clp_config(blob: &[u8]) -> Result<serde_json::Value, ClientError> {
    ZstdMsgpack::deserialize(blob).map_err(Into::into)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn creation(dataset: Option<&str>, timestamp_key: Option<&str>) -> CompressionJobCreation {
        CompressionJobCreation {
            paths: vec!["/app.log".to_owned()],
            dataset: dataset.map(str::to_owned),
            timestamp_key: timestamp_key.map(str::to_owned),
            unstructured: None,
        }
    }

    #[test]
    fn query_job_type_values() {
        assert_eq!(i32::from(QueryJobType::ExtractIr), 1);
        assert_eq!(i32::from(QueryJobType::ExtractJson), 2);
    }

    #[test]
    fn timestamp_column_names_query_is_deduplicated_and_ordered() {
        let sql = build_timestamp_column_names_query("clp_mydataset_column_metadata");

        assert_eq!(
            sql,
            "SELECT DISTINCT name FROM `clp_mydataset_column_metadata` WHERE type IN (?, ?) ORDER \
             BY name"
        );
    }

    #[test]
    fn ingestion_details_query_references_each_source_once() {
        let sql = build_ingestion_details_query(
            "SELECT MIN(begin_timestamp) AS begin_timestamp, MAX(end_timestamp) AS end_timestamp \
             FROM `clp_archives`",
            "SELECT COUNT(DISTINCT orig_file_id) AS num_files, CAST(COALESCE(SUM(num_messages), \
             0) AS SIGNED) AS num_messages FROM `clp_files`",
        );

        assert_eq!(sql.matches("`clp_archives`").count(), 1);
        assert_eq!(sql.matches("`clp_files`").count(), 1);
        assert_eq!(sql.matches("num_files").count(), 3);
    }

    #[test]
    fn ingestion_details_query_selects_the_four_expected_columns() {
        let sql = build_ingestion_details_query("SELECT 1", "SELECT 2");

        assert_eq!(
            sql,
            "SELECT a.begin_timestamp AS begin_timestamp, a.end_timestamp AS end_timestamp, \
             f.num_files AS num_files, f.num_messages AS num_messages FROM (SELECT 1) AS a, \
             (SELECT 2) AS f"
        );
    }

    #[test]
    fn normalize_listing_path_strips_redundant_components() {
        let normalized = normalize_listing_path(Path::new("/mnt/logs/./var//log"), "/mnt/logs")
            .expect("expected the path to normalize");

        assert_eq!(normalized, PathBuf::from("/mnt/logs/var/log"));
    }

    #[test]
    fn normalize_listing_path_rejects_parent_directory_components() {
        let error = normalize_listing_path(Path::new("/mnt/logs/../etc"), "/mnt/logs/../etc")
            .expect_err("a parent-directory component should be rejected");

        assert!(matches!(error, ClientError::InvalidInput(_)));
    }

    #[test]
    fn logs_input_root_check_accepts_the_configured_and_canonical_roots() {
        let configured = Path::new("/mnt/logs");
        let canonical = Path::new("/private/mnt/logs");

        assert!(is_under_logs_input_root(
            Path::new("/mnt/logs/app.log"),
            configured,
            canonical
        ));
        assert!(is_under_logs_input_root(
            Path::new("/private/mnt/logs/app.log"),
            configured,
            canonical
        ));
        assert!(is_under_logs_input_root(configured, configured, canonical));
        assert!(!is_under_logs_input_root(
            Path::new("/etc/shadow"),
            configured,
            canonical
        ));
        assert!(!is_under_logs_input_root(
            Path::new("/mnt/logsother/app.log"),
            configured,
            canonical
        ));
    }

    #[test]
    fn compression_job_creation_rejects_an_empty_path_list() {
        let mut request = creation(Some("mydataset"), None);
        request.paths.clear();

        let error = validate_compression_job_creation(&request)
            .expect_err("empty path list should be rejected");

        assert!(matches!(error, ClientError::InvalidInput(_)));
    }

    #[test]
    fn compression_job_creation_rejects_an_overlong_dataset_name() {
        let long_name = "a".repeat(clp_rust_utils::dataset::DATASET_NAME_MAX_LEN + 1);
        let request = creation(Some(&long_name), None);

        let error = validate_compression_job_creation(&request)
            .expect_err("overlong dataset name should be rejected");

        assert!(matches!(error, ClientError::InvalidDatasetName));
    }

    #[test]
    fn compression_job_config_strips_the_container_input_logs_root() {
        let config = build_compression_job_config(
            &StorageEngine::Clp,
            &ArchiveOutput::default(),
            &["/mnt/logs/var/log/app.log".to_owned()],
            &creation(None, None),
        );

        assert_eq!(config["input"]["path_prefix_to_remove"], "/mnt/logs");
        assert_eq!(config["input"]["type"], "fs");
        assert_eq!(config["input"]["unstructured"], true);
        assert!(config["input"]["dataset"].is_null());
        assert!(config["input"]["timestamp_key"].is_null());
        assert_eq!(
            config["input"]["paths_to_compress"],
            serde_json::json!(["/mnt/logs/var/log/app.log"])
        );
    }

    #[test]
    fn compression_job_config_carries_the_clp_s_fields() {
        let config = build_compression_job_config(
            &StorageEngine::ClpS,
            &ArchiveOutput::default(),
            &["/mnt/logs/app.log".to_owned()],
            &creation(Some("mydataset"), Some("ts")),
        );

        assert_eq!(config["input"]["path_prefix_to_remove"], "/mnt/logs");
        assert_eq!(config["input"]["unstructured"], false);
        assert_eq!(config["input"]["dataset"], "mydataset");
        assert_eq!(config["input"]["timestamp_key"], "ts");
    }
}
