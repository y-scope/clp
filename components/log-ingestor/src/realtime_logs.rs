use std::{
    collections::{BTreeMap, HashMap},
    fmt::Write as _,
    path::PathBuf,
    sync::Arc,
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};

use clp_rust_utils::{
    clp_config::package::{
        DEFAULT_DATASET_NAME,
        config::{ArchiveOutput, RealtimeLogs, RealtimeLogsSegment},
    },
    dataset::VALID_DATASET_NAME_REGEX,
    job_config::{ClpIoConfig, CompressionJobStatus, FsInputConfig, InputConfig, OutputConfig},
};
use non_empty_string::NonEmptyString;
use opentelemetry_proto::tonic::{
    collector::logs::v1::ExportLogsServiceRequest,
    common::v1::{AnyValue, InstrumentationScope, KeyValue, any_value},
    logs::v1::LogRecord,
    resource::v1::Resource,
};
use serde::Serialize;
use serde_json::{Map, Value, json};
use sqlx::{MySqlPool, Row, mysql::MySqlQueryResult};
use tokio::{
    fs::{self, File, OpenOptions},
    io::{AsyncWriteExt, BufWriter},
    sync::{Mutex, Semaphore},
    time,
};

use crate::ingestion_job_manager::HOT_LOG_SEGMENTS_TABLE_NAME;

const CLP_DATASET_ATTRIBUTE: &str = "clp.dataset";
const SOURCE_FORMAT_OTLP: &str = "otlp";
const STORAGE_TYPE_FS: &str = "fs";
const HOT_SEGMENT_STATUS_OPEN: i32 = 0;
const HOT_SEGMENT_STATUS_SEALED: i32 = 1;
const HOT_SEGMENT_STATUS_COMPACTING: i32 = 2;
const HOT_SEGMENT_STATUS_COMPACTED: i32 = 3;
const HOT_SEGMENT_STATUS_FAILED: i32 = 4;
const HOT_SEGMENT_STATUS_DELETED: i32 = 5;

#[derive(Clone)]
pub struct RealtimeLogManager {
    inner: Arc<Inner>,
}

struct Inner {
    db_pool: MySqlPool,
    config: RealtimeLogs,
    output_config: OutputConfig,
    hot_storage_dir: PathBuf,
    ingestion_permits: Semaphore,
    segments: Mutex<HashMap<String, ActiveSegment>>,
}

#[derive(Clone, Copy, Debug, Serialize)]
pub struct IngestSummary {
    pub records: usize,
}

#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[error("Realtime log ingestion is disabled.")]
    Disabled,
    #[error("OTLP log ingestion is disabled.")]
    OtlpDisabled,
    #[error("Invalid realtime log config: {0}")]
    InvalidConfig(String),
    #[error("Invalid dataset name: {0}")]
    InvalidDataset(String),
    #[error("No log records were provided.")]
    EmptyRequest,
    #[error(transparent)]
    Io(#[from] std::io::Error),
    #[error(transparent)]
    Sqlx(#[from] sqlx::Error),
    #[error(transparent)]
    SerdeJson(#[from] serde_json::Error),
    #[error(transparent)]
    Anyhow(#[from] anyhow::Error),
    #[error(transparent)]
    ClpRustUtils(#[from] clp_rust_utils::Error),
}

struct NormalizedRecord {
    dataset: String,
    timestamp_ms: i64,
    json_line: String,
}

struct ActiveSegment {
    id: u64,
    dataset: String,
    path: PathBuf,
    file: BufWriter<File>,
    created_at: Instant,
    size_bytes: u64,
    committed_end_offset: u64,
    record_count: u64,
    begin_timestamp: Option<i64>,
    end_timestamp: Option<i64>,
    last_file_flush_at: Instant,
    last_watermark_publish_at: Instant,
}

impl RealtimeLogManager {
    /// Creates a realtime log manager and starts the background watermark/seal task.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * The configured hot storage type is unsupported.
    /// * The configured hot storage directory can't be created.
    pub async fn new(
        db_pool: MySqlPool,
        config: RealtimeLogs,
        archive_output_config: ArchiveOutput,
    ) -> Result<Self, Error> {
        if !config.enabled {
            return Err(Error::Disabled);
        }
        if STORAGE_TYPE_FS != config.hot_storage.storage_type {
            return Err(Error::InvalidConfig(format!(
                "unsupported hot storage type `{}`",
                config.hot_storage.storage_type
            )));
        }
        validate_segment_config(&config.segment)?;

        let channel_capacity = config.segment.channel_capacity;
        let hot_storage_dir = PathBuf::from(&config.hot_storage.directory);
        fs::create_dir_all(&hot_storage_dir).await?;

        let manager = Self {
            inner: Arc::new(Inner {
                db_pool,
                output_config: OutputConfig {
                    target_archive_size: archive_output_config.target_archive_size,
                    target_dictionaries_size: archive_output_config.target_dictionaries_size,
                    target_encoded_file_size: archive_output_config.target_encoded_file_size,
                    target_segment_size: archive_output_config.target_segment_size,
                    compression_level: archive_output_config.compression_level,
                },
                config,
                hot_storage_dir,
                ingestion_permits: Semaphore::new(channel_capacity),
                segments: Mutex::new(HashMap::new()),
            }),
        };
        manager.spawn_watermark_task();
        Ok(manager)
    }

    #[must_use]
    pub fn max_request_bytes(&self) -> u64 {
        self.inner.config.otlp.max_request_bytes
    }

    /// Normalizes and appends an OTLP logs export request to hot JSONL storage.
    ///
    /// # Errors
    ///
    /// Returns an error if the OTLP pipeline is disabled, normalization fails, or segment writes
    /// fail.
    pub async fn ingest_otlp_logs(
        &self,
        request: ExportLogsServiceRequest,
    ) -> Result<IngestSummary, Error> {
        if !self.inner.config.otlp.enabled {
            return Err(Error::OtlpDisabled);
        }
        let _permit = self
            .inner
            .ingestion_permits
            .acquire()
            .await
            .map_err(|_| Error::InvalidConfig("ingestion semaphore was closed".to_string()))?;

        let records = normalize_otlp_request(request)?;
        if records.is_empty() {
            return Err(Error::EmptyRequest);
        }

        let mut by_dataset: BTreeMap<String, Vec<NormalizedRecord>> = BTreeMap::new();
        for record in records {
            by_dataset
                .entry(record.dataset.clone())
                .or_default()
                .push(record);
        }

        let mut record_count = 0;
        for (dataset, records) in by_dataset {
            record_count += records.len();
            self.append_records(&dataset, records).await?;
        }

        Ok(IngestSummary {
            records: record_count,
        })
    }

    fn spawn_watermark_task(&self) {
        let inner = self.inner.clone();
        tokio::spawn(async move {
            let interval_duration =
                Duration::from_millis(inner.config.segment.watermark_publish_interval_ms);
            let mut interval = time::interval(interval_duration);
            loop {
                interval.tick().await;
                if let Err(err) = publish_and_seal_due_segments(&inner).await {
                    tracing::warn!(
                        err = ?err,
                        "Failed to publish realtime log segment watermarks."
                    );
                }
                if let Err(err) = reconcile_compaction_results(&inner).await {
                    tracing::warn!(
                        err = ?err,
                        "Failed to reconcile realtime log segment compaction results."
                    );
                }
            }
        });
    }

    #[allow(clippy::significant_drop_tightening)]
    async fn append_records(
        &self,
        dataset: &str,
        records: Vec<NormalizedRecord>,
    ) -> Result<(), Error> {
        let mut segments = self.inner.segments.lock().await;
        if should_rotate_segment(segments.get(dataset), &self.inner.config.segment)
            && let Some(segment) = segments.remove(dataset)
        {
            seal_segment(&self.inner, segment).await?;
        }

        if !segments.contains_key(dataset) {
            let segment = create_active_segment(&self.inner, dataset).await?;
            segments.insert(dataset.to_owned(), segment);
        }

        let segment = segments
            .get_mut(dataset)
            .expect("active segment must exist after insertion");
        for record in records {
            let bytes = record.json_line.as_bytes();
            segment.file.write_all(bytes).await?;
            segment.file.write_all(b"\n").await?;
            segment.size_bytes += u64::try_from(bytes.len() + 1).map_err(|_| {
                Error::InvalidConfig("record size does not fit into u64".to_string())
            })?;
            segment.record_count += 1;
            segment.begin_timestamp = Some(min_optional_i64(
                segment.begin_timestamp,
                record.timestamp_ms,
            ));
            segment.end_timestamp =
                Some(max_optional_i64(segment.end_timestamp, record.timestamp_ms));
        }
        flush_segment_file(segment).await?;

        if should_publish_watermark_by_bytes(segment, &self.inner.config.segment) {
            publish_watermark(&self.inner.db_pool, segment).await?;
        }
        if should_rotate_segment(Some(segment), &self.inner.config.segment) {
            let segment = segments
                .remove(dataset)
                .expect("active segment must exist for sealing");
            seal_segment(&self.inner, segment).await?;
        }

        Ok(())
    }
}

async fn publish_and_seal_due_segments(inner: &Inner) -> Result<(), Error> {
    let mut segments = inner.segments.lock().await;
    let file_flush_interval = Duration::from_millis(inner.config.segment.file_flush_interval_ms);
    let watermark_publish_interval =
        Duration::from_millis(inner.config.segment.watermark_publish_interval_ms);
    let datasets_to_seal: Vec<String> = segments
        .iter()
        .filter(|&(_dataset, segment)| should_rotate_segment(Some(segment), &inner.config.segment))
        .map(|(dataset, _segment)| dataset.clone())
        .collect();

    for segment in segments.values_mut() {
        if segment.committed_end_offset < segment.size_bytes {
            if segment.last_file_flush_at.elapsed() >= file_flush_interval {
                flush_segment_file(segment).await?;
            }
            if segment.last_watermark_publish_at.elapsed() >= watermark_publish_interval {
                flush_segment_file(segment).await?;
                publish_watermark(&inner.db_pool, segment).await?;
            }
        }
    }

    for dataset in datasets_to_seal {
        if let Some(segment) = segments.remove(&dataset) {
            seal_segment(inner, segment).await?;
        }
    }
    Ok(())
}

fn validate_segment_config(config: &RealtimeLogsSegment) -> Result<(), Error> {
    if 0 == config.seal_threshold_bytes {
        return Err(Error::InvalidConfig(
            "segment.seal_threshold_bytes must be greater than 0".to_string(),
        ));
    }
    if 0 == config.seal_timeout_sec {
        return Err(Error::InvalidConfig(
            "segment.seal_timeout_sec must be greater than 0".to_string(),
        ));
    }
    if 0 == config.file_flush_interval_ms {
        return Err(Error::InvalidConfig(
            "segment.file_flush_interval_ms must be greater than 0".to_string(),
        ));
    }
    if 0 == config.watermark_publish_interval_ms {
        return Err(Error::InvalidConfig(
            "segment.watermark_publish_interval_ms must be greater than 0".to_string(),
        ));
    }
    if 0 == config.watermark_publish_threshold_bytes {
        return Err(Error::InvalidConfig(
            "segment.watermark_publish_threshold_bytes must be greater than 0".to_string(),
        ));
    }
    if 0 == config.channel_capacity {
        return Err(Error::InvalidConfig(
            "segment.channel_capacity must be greater than 0".to_string(),
        ));
    }
    Ok(())
}

fn should_rotate_segment(segment: Option<&ActiveSegment>, config: &RealtimeLogsSegment) -> bool {
    segment.is_some_and(|segment| {
        segment.size_bytes >= config.seal_threshold_bytes
            || segment.created_at.elapsed() >= Duration::from_secs(config.seal_timeout_sec)
    })
}

async fn create_active_segment(inner: &Inner, dataset: &str) -> Result<ActiveSegment, Error> {
    validate_dataset(dataset)?;
    let dataset_dir = inner.hot_storage_dir.join(dataset);
    fs::create_dir_all(&dataset_dir).await?;

    let now_ms = current_unix_time_ms();
    let path = dataset_dir.join(format!("segment-{now_ms}-{}.jsonl", std::process::id()));
    let file = OpenOptions::new()
        .create_new(true)
        .write(true)
        .open(&path)
        .await?;
    let locator = path.to_string_lossy().to_string();

    let result: MySqlQueryResult = sqlx::query(&format!(
        r"
        INSERT INTO `{HOT_LOG_SEGMENTS_TABLE_NAME}`
            (`dataset`, `status`, `storage_type`, `locator`)
        VALUES (?, ?, ?, ?)
        "
    ))
    .bind(dataset)
    .bind(HOT_SEGMENT_STATUS_OPEN)
    .bind(STORAGE_TYPE_FS)
    .bind(&locator)
    .execute(&inner.db_pool)
    .await?;

    let now = Instant::now();
    Ok(ActiveSegment {
        id: result.last_insert_id(),
        dataset: dataset.to_string(),
        path,
        file: BufWriter::new(file),
        created_at: Instant::now(),
        size_bytes: 0,
        committed_end_offset: 0,
        record_count: 0,
        begin_timestamp: None,
        end_timestamp: None,
        last_file_flush_at: now,
        last_watermark_publish_at: now,
    })
}

async fn seal_segment(inner: &Inner, mut segment: ActiveSegment) -> Result<(), Error> {
    flush_segment_file(&mut segment).await?;
    publish_watermark(&inner.db_pool, &mut segment).await?;
    sqlx::query(&format!(
        r"
        UPDATE `{HOT_LOG_SEGMENTS_TABLE_NAME}`
        SET `status` = ?, `sealed_time` = CURRENT_TIMESTAMP(3),
            `update_time` = CURRENT_TIMESTAMP(3)
        WHERE `id` = ?
        "
    ))
    .bind(HOT_SEGMENT_STATUS_SEALED)
    .bind(segment.id)
    .execute(&inner.db_pool)
    .await?;
    let compression_job_id = submit_segment_compression_job(inner, &segment).await?;
    sqlx::query(&format!(
        r"
        UPDATE `{HOT_LOG_SEGMENTS_TABLE_NAME}`
        SET `status` = ?, `compression_job_id` = ?,
            `update_time` = CURRENT_TIMESTAMP(3)
        WHERE `id` = ?
        "
    ))
    .bind(HOT_SEGMENT_STATUS_COMPACTING)
    .bind(i64::try_from(compression_job_id).unwrap_or(i64::MAX))
    .bind(segment.id)
    .execute(&inner.db_pool)
    .await?;
    tracing::info!(
        segment_id = segment.id,
        compression_job_id,
        path = %segment.path.display(),
        "Submitted realtime hot log segment for compaction."
    );
    Ok(())
}

async fn flush_segment_file(segment: &mut ActiveSegment) -> Result<(), Error> {
    segment.file.flush().await?;
    segment.last_file_flush_at = Instant::now();
    Ok(())
}

const fn should_publish_watermark_by_bytes(
    segment: &ActiveSegment,
    config: &RealtimeLogsSegment,
) -> bool {
    segment
        .size_bytes
        .saturating_sub(segment.committed_end_offset)
        >= config.watermark_publish_threshold_bytes
}

async fn submit_segment_compression_job(
    inner: &Inner,
    segment: &ActiveSegment,
) -> Result<u64, Error> {
    let dataset = NonEmptyString::new(segment.dataset.clone())
        .map_err(|_| Error::InvalidDataset(segment.dataset.clone()))?;
    let timestamp_key = NonEmptyString::new("timestamp".to_string())
        .expect("static timestamp key must be non-empty");
    let io_config = ClpIoConfig {
        input: InputConfig::FsInputConfig {
            config: FsInputConfig {
                dataset: Some(dataset),
                paths_to_compress: vec![segment.path.to_string_lossy().to_string()],
                path_prefix_to_remove: Some(inner.hot_storage_dir.to_string_lossy().to_string()),
                timestamp_key: Some(timestamp_key),
                unstructured: false,
            },
        },
        output: inner.output_config.clone(),
    };

    let result = sqlx::query(&format!(
        r"
        INSERT INTO `{}` (`clp_config`)
        VALUES (?)
        ",
        crate::compression::CLP_COMPRESSION_JOB_TABLE_NAME
    ))
    .bind(clp_rust_utils::serde::BrotliMsgpack::serialize(&io_config)?)
    .execute(&inner.db_pool)
    .await?;
    Ok(result.last_insert_id())
}

async fn reconcile_compaction_results(inner: &Inner) -> Result<(), Error> {
    let terminal_rows = sqlx::query(&format!(
        r"
        SELECT
            hot.`id` AS `hot_segment_id`,
            hot.`locator` AS `locator`,
            jobs.`status` AS `compression_status`,
            jobs.`status_msg` AS `compression_status_msg`
        FROM `{hot_table}` hot
        INNER JOIN `{compression_jobs_table}` jobs
            ON hot.`compression_job_id` = jobs.`id`
        WHERE hot.`status` = ?
            AND jobs.`status` IN (?, ?, ?)
        ",
        hot_table = HOT_LOG_SEGMENTS_TABLE_NAME,
        compression_jobs_table = crate::compression::CLP_COMPRESSION_JOB_TABLE_NAME,
    ))
    .bind(HOT_SEGMENT_STATUS_COMPACTING)
    .bind(i32::from(CompressionJobStatus::Succeeded))
    .bind(i32::from(CompressionJobStatus::Failed))
    .bind(i32::from(CompressionJobStatus::Killed))
    .fetch_all(&inner.db_pool)
    .await?;

    for row in terminal_rows {
        let hot_segment_id: i64 = row.try_get("hot_segment_id")?;
        let locator: String = row.try_get("locator")?;
        let compression_status: i32 = row.try_get("compression_status")?;
        let compression_status_msg: String = row.try_get("compression_status_msg")?;
        if compression_status == i32::from(CompressionJobStatus::Succeeded) {
            sqlx::query(&format!(
                r"
                UPDATE `{HOT_LOG_SEGMENTS_TABLE_NAME}`
                SET `status` = ?, `compacted_time` = CURRENT_TIMESTAMP(3),
                    `update_time` = CURRENT_TIMESTAMP(3)
                WHERE `id` = ?
                "
            ))
            .bind(HOT_SEGMENT_STATUS_COMPACTED)
            .bind(hot_segment_id)
            .execute(&inner.db_pool)
            .await?;
            if 0 == inner.config.compaction.raw_retention_after_compaction_sec {
                cleanup_compacted_segment_file(inner, hot_segment_id, &locator).await?;
            }
        } else {
            sqlx::query(&format!(
                r"
                UPDATE `{HOT_LOG_SEGMENTS_TABLE_NAME}`
                SET `status` = ?, `error` = ?, `update_time` = CURRENT_TIMESTAMP(3)
                WHERE `id` = ?
                "
            ))
            .bind(HOT_SEGMENT_STATUS_FAILED)
            .bind(compression_status_msg)
            .bind(hot_segment_id)
            .execute(&inner.db_pool)
            .await?;
        }
    }

    cleanup_expired_compacted_segments(inner).await?;

    Ok(())
}

async fn cleanup_expired_compacted_segments(inner: &Inner) -> Result<(), Error> {
    if 0 == inner.config.compaction.raw_retention_after_compaction_sec {
        return Ok(());
    }

    let expired_rows = sqlx::query(&format!(
        r"
        SELECT `id`, `locator`
        FROM `{HOT_LOG_SEGMENTS_TABLE_NAME}`
        WHERE `status` = ?
            AND TIMESTAMPDIFF(SECOND, `compacted_time`, CURRENT_TIMESTAMP(3)) >= ?
        "
    ))
    .bind(HOT_SEGMENT_STATUS_COMPACTED)
    .bind(
        i64::try_from(inner.config.compaction.raw_retention_after_compaction_sec)
            .unwrap_or(i64::MAX),
    )
    .fetch_all(&inner.db_pool)
    .await?;

    for row in expired_rows {
        let hot_segment_id: i64 = row.try_get("id")?;
        let locator: String = row.try_get("locator")?;
        cleanup_compacted_segment_file(inner, hot_segment_id, &locator).await?;
    }

    Ok(())
}

async fn cleanup_compacted_segment_file(
    inner: &Inner,
    hot_segment_id: i64,
    locator: &str,
) -> Result<(), Error> {
    if let Err(err) = fs::remove_file(locator).await
        && std::io::ErrorKind::NotFound != err.kind()
    {
        tracing::warn!(
            hot_segment_id,
            locator,
            err = ?err,
            "Failed to delete compacted realtime hot segment."
        );
        return Ok(());
    }
    sqlx::query(&format!(
        r"
        UPDATE `{HOT_LOG_SEGMENTS_TABLE_NAME}`
        SET `status` = ?, `update_time` = CURRENT_TIMESTAMP(3)
        WHERE `id` = ?
        "
    ))
    .bind(HOT_SEGMENT_STATUS_DELETED)
    .bind(hot_segment_id)
    .execute(&inner.db_pool)
    .await?;
    tracing::info!(
        hot_segment_id,
        locator,
        "Deleted compacted realtime hot segment."
    );
    Ok(())
}

async fn publish_watermark(db_pool: &MySqlPool, segment: &mut ActiveSegment) -> Result<(), Error> {
    sqlx::query(&format!(
        r"
        UPDATE `{HOT_LOG_SEGMENTS_TABLE_NAME}`
        SET `committed_end_offset` = ?, `size_bytes` = ?, `record_count` = ?,
            `begin_timestamp` = ?, `end_timestamp` = ?,
            `update_time` = CURRENT_TIMESTAMP(3)
        WHERE `id` = ?
        "
    ))
    .bind(i64::try_from(segment.size_bytes).unwrap_or(i64::MAX))
    .bind(i64::try_from(segment.size_bytes).unwrap_or(i64::MAX))
    .bind(i64::try_from(segment.record_count).unwrap_or(i64::MAX))
    .bind(segment.begin_timestamp)
    .bind(segment.end_timestamp)
    .bind(segment.id)
    .execute(db_pool)
    .await?;
    segment.committed_end_offset = segment.size_bytes;
    segment.last_watermark_publish_at = Instant::now();
    Ok(())
}

fn normalize_otlp_request(
    request: ExportLogsServiceRequest,
) -> Result<Vec<NormalizedRecord>, Error> {
    let ingest_time = current_unix_time_ms();
    let mut records = Vec::new();

    for resource_logs in request.resource_logs {
        let resource = resource_logs.resource.as_ref();
        let resource_attributes = resource
            .map(|resource| attributes_to_json_object(&resource.attributes))
            .unwrap_or_default();
        for scope_logs in resource_logs.scope_logs {
            let scope = scope_logs.scope.as_ref();
            let scope_json = scope_to_json(scope);
            for log_record in scope_logs.log_records {
                let dataset = resolve_dataset(&log_record, resource)?;
                validate_dataset(&dataset)?;
                let timestamp_ms = normalize_timestamp_ms(&log_record, ingest_time);
                let attributes = attributes_to_json_object(&log_record.attributes);
                let body = log_record
                    .body
                    .as_ref()
                    .map_or(Value::Null, any_value_to_json);

                let mut object = Map::new();
                object.insert("timestamp".to_string(), json!(timestamp_ms));
                object.insert(
                    "observed_timestamp".to_string(),
                    json!(
                        nanos_to_millis(log_record.observed_time_unix_nano).unwrap_or(ingest_time)
                    ),
                );
                object.insert("severity_text".to_string(), json!(log_record.severity_text));
                object.insert(
                    "severity_number".to_string(),
                    json!(log_record.severity_number),
                );
                object.insert("body".to_string(), body);
                object.insert(
                    "trace_id".to_string(),
                    json!(bytes_to_hex(&log_record.trace_id)),
                );
                object.insert(
                    "span_id".to_string(),
                    json!(bytes_to_hex(&log_record.span_id)),
                );
                object.insert("attributes".to_string(), Value::Object(attributes));
                object.insert(
                    "resource".to_string(),
                    Value::Object(resource_attributes.clone()),
                );
                object.insert("scope".to_string(), scope_json.clone());
                object.insert(
                    "_clp".to_string(),
                    json!({
                        "ingest_time": ingest_time,
                        "source_format": SOURCE_FORMAT_OTLP,
                        "dataset": dataset.clone(),
                    }),
                );

                records.push(NormalizedRecord {
                    dataset,
                    timestamp_ms,
                    json_line: serde_json::to_string(&Value::Object(object))?,
                });
            }
        }
    }

    Ok(records)
}

fn resolve_dataset(log_record: &LogRecord, resource: Option<&Resource>) -> Result<String, Error> {
    let dataset = find_string_attribute(&log_record.attributes, CLP_DATASET_ATTRIBUTE)
        .or_else(|| {
            resource.and_then(|resource| {
                find_string_attribute(&resource.attributes, CLP_DATASET_ATTRIBUTE)
            })
        })
        .unwrap_or_else(|| DEFAULT_DATASET_NAME.to_string());
    validate_dataset(&dataset)?;
    Ok(dataset)
}

fn validate_dataset(dataset: &str) -> Result<(), Error> {
    if VALID_DATASET_NAME_REGEX.is_match(dataset) {
        return Ok(());
    }
    Err(Error::InvalidDataset(dataset.to_string()))
}

fn normalize_timestamp_ms(log_record: &LogRecord, ingest_time: i64) -> i64 {
    nanos_to_millis(log_record.time_unix_nano)
        .or_else(|| nanos_to_millis(log_record.observed_time_unix_nano))
        .unwrap_or(ingest_time)
}

fn nanos_to_millis(timestamp_nano: u64) -> Option<i64> {
    (0 != timestamp_nano).then(|| i64::try_from(timestamp_nano / 1_000_000).unwrap_or(i64::MAX))
}

fn scope_to_json(scope: Option<&InstrumentationScope>) -> Value {
    let Some(scope) = scope else {
        return Value::Object(Map::new());
    };
    json!({
        "name": scope.name.clone(),
        "version": scope.version.clone(),
        "attributes": attributes_to_json_object(&scope.attributes),
    })
}

fn attributes_to_json_object(attributes: &[KeyValue]) -> Map<String, Value> {
    attributes
        .iter()
        .map(|attribute| {
            (
                attribute.key.clone(),
                any_value_to_json_option(attribute.value.as_ref()),
            )
        })
        .collect()
}

fn find_string_attribute(attributes: &[KeyValue], key: &str) -> Option<String> {
    attributes
        .iter()
        .find(|attribute| attribute.key == key)
        .and_then(
            |attribute| match attribute.value.as_ref()?.value.as_ref()? {
                any_value::Value::StringValue(value) => Some(value.clone()),
                _ => None,
            },
        )
}

fn any_value_to_json_option(value: Option<&AnyValue>) -> Value {
    value.map_or(Value::Null, any_value_to_json)
}

fn any_value_to_json(value: &AnyValue) -> Value {
    match value.value.as_ref() {
        Some(any_value::Value::StringValue(value)) => json!(value),
        Some(any_value::Value::StringValueStrindex(value)) => json!(value),
        Some(any_value::Value::BoolValue(value)) => json!(value),
        Some(any_value::Value::IntValue(value)) => json!(value),
        Some(any_value::Value::DoubleValue(value)) => json!(value),
        Some(any_value::Value::ArrayValue(value)) => Value::Array(
            value
                .values
                .iter()
                .map(any_value_to_json)
                .collect::<Vec<Value>>(),
        ),
        Some(any_value::Value::KvlistValue(value)) => Value::Object(
            value
                .values
                .iter()
                .map(|attribute| {
                    (
                        attribute.key.clone(),
                        any_value_to_json_option(attribute.value.as_ref()),
                    )
                })
                .collect(),
        ),
        Some(any_value::Value::BytesValue(value)) => json!(bytes_to_hex(value)),
        None => Value::Null,
    }
}

fn bytes_to_hex(bytes: &[u8]) -> String {
    let mut output = String::with_capacity(bytes.len() * 2);
    for byte in bytes {
        let _ = write!(output, "{byte:02x}");
    }
    output
}

fn current_unix_time_ms() -> i64 {
    i64::try_from(
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or(Duration::ZERO)
            .as_millis(),
    )
    .unwrap_or(i64::MAX)
}

fn min_optional_i64(current: Option<i64>, next: i64) -> i64 {
    current.map_or(next, |current| current.min(next))
}

fn max_optional_i64(current: Option<i64>, next: i64) -> i64 {
    current.map_or(next, |current| current.max(next))
}
